package main

import (
	"bytes"
	"crypto/sha256"
	"digisocialblock/core/content"
	"digisocialblock/core/identity"
	"digisocialblock/core/user"
	"digisocialblock/pkg/dds/chunking" // Assuming this path
	"encoding/hex"
	"fmt"
	"io"
	"log"
	"reflect"
	"sync"
	// "time" // Not strictly needed for this main, but profile sets timestamp
)

// --- Mock DDS Implementations (Copied from test_dds_ledger_integration/main.go for standalone testability) ---

// MockChunker is a simple mock for DDSChunker interface.
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
	for _, ci := range manifest.Chunks { // Order matters for this simple manifest CID
		cidBuffer.WriteString(ci.ChunkCID)
	}
	manifestHash := sha256.Sum256(cidBuffer.Bytes())
	manifest.ManifestCID = "test_manifest_" + hex.EncodeToString(manifestHash[:])
	manifest.EncryptionMethod = "none"
	mc.StoredManifests[manifest.ManifestCID] = manifest
	return manifest, dataChunks, nil
}

// MockStorage is a simple in-memory mock for DDSStorage/DDSChunkRetriever interface.
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

// MockManifestFetcher retrieves manifests stored by MockChunker.
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
	log.Println("--- Test User Profile DDS Integration Scenario ---")

	// 1. Setup Wallet for a user
	wallet, err := identity.NewWallet()
	if err != nil {
		log.Fatalf("Failed to create wallet: %v", err)
	}
	log.Printf("User Wallet created. Address (OwnerPublicKey): %s", wallet.Address)

	// 2. Setup DDS components (mocks)
	mockChunker := NewMockChunker(128) // Smaller chunk size for profile data
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
	profileManager, err := user.NewProfileManager(contentPublisher, contentRetriever)
	if err != nil {
		log.Fatalf("Failed to create profile manager: %v", err)
	}
	log.Println("ProfileManager and mock DDS components initialized.")

	// 3. Create and Publish Initial Profile
	log.Println("\n--- Creating and Publishing Initial Profile ---")
	initialProfile := user.NewProfile(wallet.Address, "JulesTheDeveloper", "Loves building decentralized systems and exploring AI frontiers.")
	initialProfile.ProfilePictureCID = "placeholder_pic_cid_1" // Just a string for now
	initialProfile.HeaderImageCID = "placeholder_header_cid_1"

	log.Printf("Initial Profile Data: %+v", initialProfile)

	profileCID1, err := profileManager.PublishProfile(initialProfile)
	if err != nil {
		log.Fatalf("Failed to publish initial profile: %v", err)
	}
	log.Printf("Initial profile published to DDS. Profile CID: %s", profileCID1)

	// 4. Retrieve and Verify Initial Profile
	log.Println("\n--- Retrieving and Verifying Initial Profile ---")
	retrievedProfile1, err := profileManager.RetrieveProfile(profileCID1)
	if err != nil {
		log.Fatalf("Failed to retrieve initial profile (CID: %s): %v", profileCID1, err)
	}
	log.Printf("Retrieved Profile 1 Data: %+v", retrievedProfile1)

	if !reflect.DeepEqual(initialProfile, retrievedProfile1) {
		log.Fatalf("Retrieved profile 1 does not match original!\nOriginal: %+v\nRetrieved: %+v", initialProfile, retrievedProfile1)
	}
	log.Println("Successfully retrieved and verified initial profile.")

	// 5. Update and Publish New Profile Version (Optional Demonstration)
	log.Println("\n--- Updating and Publishing New Profile Version ---")
	// Simulate time passing and getting the profile again to update it
	// In a real app, you'd fetch the existing profile by a known reference (e.g., user's latest profile CID)
	// For this test, we'll just use `initialProfile` and call `Update`
	updated := initialProfile.Update("JulesTheArchitect", "Still loves building, but now with more architecture!", "new_pic_cid_v2", "")
	if !updated {
		log.Fatalf("Profile update reported no changes, but changes were made.")
	}
	log.Printf("Updated Profile Data (Version %d): %+v", initialProfile.Version, initialProfile)

	profileCID2, err := profileManager.PublishProfile(initialProfile) // initialProfile is now the updated one
	if err != nil {
		log.Fatalf("Failed to publish updated profile: %v", err)
	}
	if profileCID1 == profileCID2 {
		log.Fatalf("Updated profile resulted in the same CID as original. This should not happen if content changed. CID1: %s, CID2: %s", profileCID1, profileCID2)
	}
	log.Printf("Updated profile published to DDS. New Profile CID: %s", profileCID2)

	// 6. Retrieve and Verify Updated Profile
	log.Println("\n--- Retrieving and Verifying Updated Profile ---")
	retrievedProfile2, err := profileManager.RetrieveProfile(profileCID2)
	if err != nil {
		log.Fatalf("Failed to retrieve updated profile (CID: %s): %v", profileCID2, err)
	}
	log.Printf("Retrieved Profile 2 Data: %+v", retrievedProfile2)

	if !reflect.DeepEqual(initialProfile, retrievedProfile2) { // initialProfile is the updated one here
		log.Fatalf("Retrieved profile 2 does not match updated original!\nOriginalUpdated: %+v\nRetrieved2: %+v", initialProfile, retrievedProfile2)
	}
	if retrievedProfile2.Version != 2 {
		log.Fatalf("Expected updated profile version to be 2, got %d", retrievedProfile2.Version)
	}
	log.Println("Successfully retrieved and verified updated profile.")


	log.Println("\n--- Test User Profile DDS Integration Scenario Complete ---")
}
