package main

import (
	"bytes"
	"crypto/sha256"
	"digisocialblock/core/content"
	"digisocialblock/core/identity"
	"digisocialblock/core/ledger"
	"digisocialblock/pkg/dds/chunking" // Assuming these paths
	"encoding/hex"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"sync"
)

// --- Mock DDS Implementations (for this test scenario) ---

// MockChunker is a simple mock for DDSChunker interface.
type MockChunker struct {
	DefaultChunkSize int
	StoredManifests  map[string]*chunking.ContentManifestV1 // Store generated manifests by CID
	mu               sync.Mutex
}

func NewMockChunker(chunkSize int) *MockChunker {
	if chunkSize <= 0 {
		chunkSize = 1024 // Default if invalid
	}
	return &MockChunker{
		DefaultChunkSize: chunkSize,
		StoredManifests:  make(map[string]*chunking.ContentManifestV1),
	}
}

func (mc *MockChunker) ChunkData(data io.Reader) (*chunking.ContentManifestV1, []chunking.DataChunk, error) {
	mc.mu.Lock()
	defer mc.mu.Unlock()

	var allData []byte
	buf := make([]byte, 256) // Read in smaller segments
	for {
		n, err := data.Read(buf)
		if n > 0 {
			allData = append(allData, buf[:n]...)
		}
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, nil, fmt.Errorf("mock chunker failed to read data: %w", err)
		}
	}

	if len(allData) == 0 {
		// Handle empty data by creating a specific manifest for it
		manifest := &chunking.ContentManifestV1{
			Version:          1,
			TotalSize:        0,
			Chunks:           []chunking.ChunkInfo{},
			ManifestCID:      "empty_content_manifest_cid_v1", // Fixed CID for empty content
			EncryptionMethod: "none",
		}
		mc.StoredManifests[manifest.ManifestCID] = manifest
		return manifest, []chunking.DataChunk{}, nil
	}

	manifest := &chunking.ContentManifestV1{
		Version:   1,
		TotalSize: int64(len(allData)),
	}
	var dataChunks []chunking.DataChunk
	var chunkInfos []chunking.ChunkInfo


	for i := 0; i < len(allData); i += mc.DefaultChunkSize {
		end := i + mc.DefaultChunkSize
		if end > len(allData) {
			end = len(allData)
		}
		chunkData := allData[i:end]

		hash := sha256.Sum256(chunkData)
		chunkCID := hex.EncodeToString(hash[:])

		dc := chunking.DataChunk{
			ChunkCID: chunkCID,
			Data:     chunkData,
			Size:     int64(len(chunkData)),
		}
		dataChunks = append(dataChunks, dc)
		chunkInfos = append(chunkInfos, chunking.ChunkInfo{ChunkCID: chunkCID, Size: int64(len(chunkData))})
	}
	manifest.Chunks = chunkInfos


	// Simple Manifest CID: hash of concatenated (sorted) chunk CIDs for simplicity & determinism
	var sortedChunkCIDs []string
	for _, ci := range manifest.Chunks {
		sortedChunkCIDs = append(sortedChunkCIDs, ci.ChunkCID)
	}
	// Sort is important if the order of chunks in manifest.Chunks isn't guaranteed or if CID relies on it.
	// For this mock, let's assume our chunking process produces them in order, but for CID calc, sort.
	// sort.Strings(sortedChunkCIDs) // Not strictly needed if manifest.Chunks is always ordered.

	var cidBuffer bytes.Buffer
	for _, chunkCID := range sortedChunkCIDs { // Iterate over sorted CIDs for manifest CID calculation
		cidBuffer.WriteString(chunkCID)
	}
	manifestHash := sha256.Sum256(cidBuffer.Bytes())
	// Prefix "test_manifest_" to distinguish from actual chunk CIDs if they could collide.
	manifest.ManifestCID = "test_manifest_" + hex.EncodeToString(manifestHash[:])
	manifest.EncryptionMethod = "none"

	mc.StoredManifests[manifest.ManifestCID] = manifest // Store for mock fetcher
	return manifest, dataChunks, nil
}


// MockStorage is a simple in-memory mock for DDSStorage/DDSChunkRetriever interface.
type MockStorage struct {
	mu     sync.Mutex
	chunks map[string][]byte
}

func NewMockStorage() *MockStorage {
	return &MockStorage{
		chunks: make(map[string][]byte),
	}
}

func (ms *MockStorage) StoreChunk(chunkID string, data []byte) error {
	ms.mu.Lock()
	defer ms.mu.Unlock()
	if chunkID == "" {
		return fmt.Errorf("mock storage: chunkID cannot be empty")
	}
	ms.chunks[chunkID] = bytes.Clone(data)
	return nil
}

func (ms *MockStorage) RetrieveChunk(chunkID string) ([]byte, error) {
	ms.mu.Lock()
	defer ms.mu.Unlock()
	data, ok := ms.chunks[chunkID]
	if !ok {
		return nil, fmt.Errorf("mock storage: chunk %s not found", chunkID)
	}
	return bytes.Clone(data), nil
}

func (ms *MockStorage) ChunkExists(chunkID string) bool {
	ms.mu.Lock()
	defer ms.mu.Unlock()
	_, ok := ms.chunks[chunkID]
	return ok
}

// MockManifestFetcher retrieves manifests stored by MockChunker.
type MockManifestFetcher struct {
	chunkerRef *MockChunker // Reference to the chunker that stores manifests
}

func NewMockManifestFetcher(chunker *MockChunker) *MockManifestFetcher {
	return &MockManifestFetcher{chunkerRef: chunker}
}

func (mmf *MockManifestFetcher) FetchManifest(manifestCID string) (*chunking.ContentManifestV1, error) {
	mmf.chunkerRef.mu.Lock() // Need to lock if accessing shared StoredManifests
	defer mmf.chunkerRef.mu.Unlock()

	manifest, ok := mmf.chunkerRef.StoredManifests[manifestCID]
	if !ok {
		return nil, fmt.Errorf("mock manifest fetcher: manifest CID %s not found", manifestCID)
	}
	// Return a copy to prevent modification of the stored manifest by the retriever
	manifestCopy := *manifest
	manifestCopy.Chunks = make([]chunking.ChunkInfo, len(manifest.Chunks))
	copy(manifestCopy.Chunks, manifest.Chunks)
	return &manifestCopy, nil
}


func main() {
	log.Println("--- Test DDS Ledger Integration Scenario ---")

	// 1. Setup Wallet
	wallet, err := identity.NewWallet()
	if err != nil {
		log.Fatalf("Failed to create wallet: %v", err)
	}
	log.Printf("Wallet created. Address: %s", wallet.Address)

	// 2. Setup DDS components (using mocks)
	mockChunker := NewMockChunker(1024) // 1KB chunk size for testing
	mockStorage := NewMockStorage()
	mockOriginator := &content.SimplePlaceholderOriginator{} // Using the one from content package

	contentPublisher, err := content.NewContentPublisher(mockChunker, mockStorage, mockOriginator)
	if err != nil {
		log.Fatalf("Failed to create content publisher: %v", err)
	}
	log.Println("ContentPublisher initialized with mock DDS components.")

	// 3. Sample post text
	postText := "This is a test post for Digisocialblock! " +
		"It demonstrates integrating DDS content addressing with the ledger. " +
		"The content itself will be chunked, stored in a mock DDS, and its manifest CID " +
		"will be recorded in a transaction on the blockchain. This ensures that the ledger " +
		"remains lightweight while content can be stored decentrally. " +
		"This is a fairly long string to ensure it gets split into multiple chunks with a 1KB chunk size. " +
		"Let's add more text to make sure. The quick brown fox jumps over the lazy dog. " +
		"Repeating this sentence multiple times to increase length. " +
		"The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. " +
		"The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. " +
		"The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. " +
		"This should definitely be enough now for several chunks to be created by our mock chunker setup."

	log.Printf("\nPublishing post content to mock DDS:\n\"%s...\"\n", postText[:100])

	// 4. Publish content to DDS to get CID
	cid, err := contentPublisher.PublishTextPostToDDS(postText)
	if err != nil {
		log.Fatalf("Failed to publish content to DDS: %v", err)
	}
	log.Printf("Content published to DDS. Manifest CID: %s", cid)

	// 5. Create a "PostCreated" transaction
	log.Println("\nCreating 'PostCreated' transaction...")
	tx, err := ledger.NewTransaction(wallet.Address, ledger.PostCreated, []byte(cid))
	if err != nil {
		log.Fatalf("Failed to create new transaction: %v", err)
	}
	log.Printf("Transaction created. ID: %s, Type: %s, Payload (CID): %s", tx.ID, tx.Type, string(tx.Payload))

	// 6. Sign the transaction
	log.Println("Signing transaction...")
	if err := wallet.SignTransaction(tx); err != nil {
		log.Fatalf("Failed to sign transaction: %v", err)
	}
	log.Printf("Transaction signed. Signature (first 16 bytes): %x...", tx.Signature[:16])

	// Verify signature (optional check here, AddBlock will also do it)
	validSig, err := tx.VerifySignature()
	if err != nil || !validSig {
		log.Fatalf("Transaction signature is invalid after signing: %v (valid: %t)", err, validSig)
	}
	log.Println("Transaction signature verified successfully (locally).")

	// 7. Initialize Blockchain
	log.Println("\nInitializing blockchain...")
	bc, err := ledger.NewBlockchain()
	if err != nil {
		log.Fatalf("Failed to initialize blockchain: %v", err)
	}
	log.Printf("Blockchain initialized. Genesis block hash: %s", bc.GetLatestBlock().Hash)

	// 8. Add transaction to a new block
	log.Println("Adding transaction to a new block...")
	newBlock, err := bc.AddBlock([]*ledger.Transaction{tx})
	if err != nil {
		log.Fatalf("Failed to add block to blockchain: %v", err)
	}
	log.Printf("Block #%d added. Hash: %s", newBlock.Index, newBlock.Hash)
	log.Printf("  Contains %d transaction(s).", len(newBlock.Transactions))
	for i, blockTx := range newBlock.Transactions {
		log.Printf("  Tx %d: ID=%s, Type=%s, Sender=%s..., Payload(CID)=%s",
			i, blockTx.ID, blockTx.Type, blockTx.SenderPublicKey[:10], string(blockTx.Payload))
	}

	// 9. Validate chain (optional check)
	isChainValid, chainErr := bc.IsChainValid()
	if chainErr != nil || !isChainValid {
		log.Fatalf("Blockchain validation failed: %v (valid: %t)", chainErr, isChainValid)
	}
	log.Println("\nBlockchain is valid.")

	log.Println("\n--- Test DDS Ledger Integration Scenario Complete ---")

	// --- Part 2: Simulate Social Feed Retrieval ---
	log.Println("\n--- Simulating Social Feed Retrieval ---")

	// Initialize ContentRetriever with the mock components
	mockManifestFetcher := NewMockManifestFetcher(mockChunker) // Pass the same chunker instance
	// mockStorage is already initialized and populated from the publishing phase
	contentRetriever, err := content.NewContentRetriever(mockManifestFetcher, mockStorage)
	if err != nil {
		log.Fatalf("Failed to create content retriever: %v", err)
	}
	log.Println("ContentRetriever initialized.")

	// Iterate through blockchain to find PostCreated transactions and retrieve content
	for blockIdx, block := range bc.Blocks {
		if block == nil {
			log.Printf("Skipping nil block at index %d", blockIdx)
			continue
		}
		log.Printf("\nProcessing Block #%d (Hash: %s...)", block.Index, block.Hash[:10])
		for txIdx, tx := range block.Transactions {
			if tx == nil {
				log.Printf("Skipping nil transaction at block %d, tx index %d", blockIdx, txIdx)
				continue
			}
			if tx.Type == ledger.PostCreated {
				postCID := string(tx.Payload)
				log.Printf("  Found PostCreated Transaction (ID: %s...) with CID: %s", tx.ID[:10], postCID)
				log.Printf("  Attempting to retrieve content for CID: %s", postCID)

				retrievedPostText, err := contentRetriever.RetrieveAndVerifyTextPost(postCID)
				if err != nil {
					log.Printf("    ERROR retrieving content for CID %s: %v", postCID, err)
				} else {
					log.Printf("    Successfully retrieved content:\n      \"%s...\"\n", retrievedPostText[:min(150, len(retrievedPostText))])
					// Verify against original if we want to be thorough (requires storing original texts)
					if postCID == cid && retrievedPostText != postText { // 'cid' and 'postText' are from the publishing part
						log.Printf("    VERIFICATION MISMATCH: Retrieved content differs from original published content for CID %s!", postCID)
					} else if postCID == cid {
						log.Println("    Retrieved content matches original published content.")
					}
				}
			}
		}
	}

	log.Println("\n--- Social Feed Simulation Complete ---")


}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

// Helper to set up a temporary directory for DDS storage if using file-based mock
func setupTempDir(t *testing.T) string {
	dir, err := os.MkdirTemp("", "dds_test_storage_")
	if err != nil {
		t.Fatalf("Failed to create temp dir: %v", err)
	}
	return dir
}

// Helper to create a simple file based storage for testing (if needed)
type FileMockStorage struct {
	basePath string
}
func NewFileMockStorage(basePath string) (*FileMockStorage, error) {
	if err := os.MkdirAll(basePath, 0755); err != nil {
		return nil, err
	}
	return &FileMockStorage{basePath: basePath}, nil
}
func (fs *FileMockStorage) StoreChunk(chunkID string, data []byte) error {
	return os.WriteFile(filepath.Join(fs.basePath, chunkID), data, 0644)
}
func (fs *FileMockStorage) RetrieveChunk(chunkID string) ([]byte, error) {
	return os.ReadFile(filepath.Join(fs.basePath, chunkID))
}
func (fs *FileMockStorage) ChunkExists(chunkID string) bool {
	_, err := os.Stat(filepath.Join(fs.basePath, chunkID))
	return !os.IsNotExist(err)
}
