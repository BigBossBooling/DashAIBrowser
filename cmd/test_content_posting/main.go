package main

import (
	"bytes"
	"crypto/sha256"
	"digisocialblock/core/content"
	"digisocialblock/core/identity"
	"digisocialblock/core/ledger"
	"digisocialblock/core/social"
	"digisocialblock/pkg/dds/chunking" // Assuming this path
	"encoding/hex"
	"fmt"
	"io"
	"log"
	// "reflect" // Not strictly needed for this test's core verification
	"sync"
	// "time"
)

// --- Mock DDS Implementations (Copied for standalone testability) ---

// MockChunker
type MockChunker struct {
	DefaultChunkSize int
	StoredManifests  map[string]*chunking.ContentManifestV1
	mu               sync.Mutex
}

func NewMockChunker(chunkSize int) *MockChunker {
	if chunkSize <= 0 {
		chunkSize = 1024
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
	buf := make([]byte, 256)
	for {
		n, err := data.Read(buf)
		if n > 0 {
			allData = append(allData, buf[:n]...)
		}
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, nil, fmt.Errorf("mock chunker read error: %w", err)
		}
	}
	if len(allData) == 0 {
		manifest := &chunking.ContentManifestV1{
			Version: 1, TotalSize: 0, Chunks: []chunking.ChunkInfo{},
			ManifestCID: "empty_content_manifest_cid_v1", EncryptionMethod: "none",
		}
		mc.StoredManifests[manifest.ManifestCID] = manifest
		return manifest, []chunking.DataChunk{}, nil
	}
	manifest := &chunking.ContentManifestV1{Version: 1, TotalSize: int64(len(allData))}
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
		dataChunks = append(dataChunks, chunking.DataChunk{ChunkCID: chunkCID, Data: chunkData, Size: int64(len(chunkData))})
		chunkInfos = append(chunkInfos, chunking.ChunkInfo{ChunkCID: chunkCID, Size: int64(len(chunkData))})
	}
	manifest.Chunks = chunkInfos
	var cidBuffer bytes.Buffer
	for _, ci := range manifest.Chunks {
		cidBuffer.WriteString(ci.ChunkCID)
	}
	manifestHash := sha256.Sum256(cidBuffer.Bytes())
	manifest.ManifestCID = "test_content_manifest_" + hex.EncodeToString(manifestHash[:]) // Different prefix
	manifest.EncryptionMethod = "none"
	mc.StoredManifests[manifest.ManifestCID] = manifest
	return manifest, dataChunks, nil
}

// MockStorage
type MockStorage struct {
	mu     sync.Mutex
	chunks map[string][]byte
}

func NewMockStorage() *MockStorage {
	return &MockStorage{chunks: make(map[string][]byte)}
}
func (ms *MockStorage) StoreChunk(chunkID string, data []byte) error {
	ms.mu.Lock()
	defer ms.mu.Unlock()
	if chunkID == "" {
		return fmt.Errorf("mock storage: chunkID empty")
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

// MockManifestFetcher
type MockManifestFetcher struct {
	chunkerRef *MockChunker
}

func NewMockManifestFetcher(chunker *MockChunker) *MockManifestFetcher {
	return &MockManifestFetcher{chunkerRef: chunker}
}
func (mmf *MockManifestFetcher) FetchManifest(manifestCID string) (*chunking.ContentManifestV1, error) {
	mmf.chunkerRef.mu.Lock()
	defer mmf.chunkerRef.mu.Unlock()
	manifest, ok := mmf.chunkerRef.StoredManifests[manifestCID]
	if !ok {
		return nil, fmt.Errorf("mock manifest fetcher: manifest %s not found", manifestCID)
	}
	manifestCopy := *manifest
	manifestCopy.Chunks = make([]chunking.ChunkInfo, len(manifest.Chunks))
	copy(manifestCopy.Chunks, manifest.Chunks)
	return &manifestCopy, nil
}

func main() {
	log.Println("--- Test Content Posting Scenario ---")

	// 1. Setup Wallet
	wallet, err := identity.NewWallet()
	if err != nil {
		log.Fatalf("Failed to create wallet: %v", err)
	}
	log.Printf("User Wallet created. Address: %s", wallet.Address)

	// 2. Setup DDS components & Managers
	mockChunker := NewMockChunker(100) // Small chunk size for content
	mockStorage := NewMockStorage()
	mockOriginator := &content.SimplePlaceholderOriginator{}
	mockManifestFetcher := NewMockManifestFetcher(mockChunker)

	contentPublisher, err := content.NewContentPublisher(mockChunker, mockStorage, mockOriginator)
	if err != nil {
		log.Fatalf("Failed to create content publisher: %v", err)
	}
	contentRetriever, err := content.NewContentRetriever(mockManifestFetcher, mockStorage)
	if err != nil {
		log.Fatalf("Failed to create content retriever: %v", err)
	}
	postManager, err := social.NewPostManager(contentPublisher)
	if err != nil {
		log.Fatalf("Failed to create post manager: %v", err)
	}
	log.Println("Managers and mock DDS components initialized.")

	// 3. Initialize Blockchain
	bc, err := ledger.NewBlockchain()
	if err != nil {
		log.Fatalf("Failed to initialize blockchain: %v", err)
	}
	log.Printf("Blockchain initialized. Genesis block hash: %s", bc.GetLatestBlock().Hash)

	// 4. Create and Publish a Post
	originalPostText := "This is the first post on Digisocialblock! Stored on DDS, referenced on ledger."
	postTitle := "My First Post"
	postTags := []string{"gola", "decentralized", "social"}

	log.Printf("\nCreating post with text: \"%s...\"", originalPostText[:30])
	postTx, err := postManager.CreatePost(wallet, originalPostText, postTitle, postTags)
	if err != nil {
		log.Fatalf("Failed to create post: %v", err)
	}
	log.Printf("Post creation transaction generated. Tx ID: %s", postTx.ID)

	// 5. Add the transaction to the blockchain
	log.Println("Adding post transaction to blockchain...")
	newBlock, err := bc.AddBlock([]*ledger.Transaction{postTx})
	if err != nil {
		log.Fatalf("Failed to add block with post transaction: %v", err)
	}
	log.Printf("Block #%d added. Hash: %s...", newBlock.Index, newBlock.Hash[:10])

	// 6. Verify transaction and retrieve content
	log.Println("\nVerifying transaction in block and retrieving content...")
	foundAndVerified := false
	for _, txInBlock := range newBlock.Transactions {
		if txInBlock.ID == postTx.ID {
			log.Printf("  Found transaction %s in block.", txInBlock.ID)
			if txInBlock.Type != ledger.PostCreated {
				log.Fatalf("  Transaction type is %s, expected PostCreated", txInBlock.Type)
			}

			// Deserialize Post metadata from payload
			postMeta, err := social.PostFromJSON(txInBlock.Payload)
			if err != nil {
				log.Fatalf("  Failed to deserialize Post metadata from transaction payload: %v", err)
			}
			log.Printf("  Deserialized Post Metadata: Author=%s..., ContentCID=%s, Title=%s",
				postMeta.AuthorPublicKey[:10], postMeta.ContentCID, postMeta.Title)

			if postMeta.AuthorPublicKey != wallet.Address {
				log.Fatalf("  Post author mismatch. Expected %s, got %s", wallet.Address, postMeta.AuthorPublicKey)
			}

			// Retrieve content from DDS using ContentCID from post metadata
			log.Printf("  Attempting to retrieve content from DDS using CID: %s", postMeta.ContentCID)
			retrievedContent, err := contentRetriever.RetrieveAndVerifyTextPost(postMeta.ContentCID)
			if err != nil {
				log.Fatalf("  Failed to retrieve content from DDS: %v", err)
			}

			if retrievedContent == originalPostText {
				log.Printf("  SUCCESS! Retrieved content matches original post text:\n    \"%s...\"", retrievedContent[:min(100, len(retrievedContent))])
				foundAndVerified = true
			} else {
				log.Fatalf("  CONTENT MISMATCH! \nOriginal: %s\nRetrieved: %s", originalPostText, retrievedContent)
			}
			break
		}
	}
	if !foundAndVerified {
		log.Fatalf("Post transaction %s was not found in the new block or content verification failed.", postTx.ID)
	}

	// 7. Validate chain
	isValid, chainErr := bc.IsChainValid()
	if chainErr != nil || !isValid {
		log.Fatalf("Blockchain validation failed: %v (valid: %t)", chainErr, isValid)
	}
	log.Println("\nBlockchain is valid after adding post.")

	log.Println("\n--- Test Content Posting Scenario Complete ---")
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
