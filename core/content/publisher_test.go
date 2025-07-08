package content

import (
	"bytes"
	"crypto/sha256"
	"digisocialblock/pkg/dds/chunking" // Assuming this path
	"encoding/hex"
	"fmt"
	"io"
	"sync"
	"testing"
)

// --- Mock DDS Implementations for Testing ---

// MockTestChunker is a mock for DDSChunker interface used in tests.
type MockTestChunker struct {
	ChunkSize      int
	SimulateError  bool
	ExpectedChunks int // Optional: for verifying number of chunks
}

func (mtc *MockTestChunker) ChunkData(data io.Reader) (*chunking.ContentManifestV1, []chunking.DataChunk, error) {
	if mtc.SimulateError {
		return nil, nil, fmt.Errorf("mock chunker simulated error")
	}

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
			return nil, nil, fmt.Errorf("mock chunker failed to read data: %w", err)
		}
	}
	if len(allData) == 0 {
		return &chunking.ContentManifestV1{ManifestCID: "empty_data_manifest_cid"}, []chunking.DataChunk{}, nil
	}


	manifest := &chunking.ContentManifestV1{
		Version:   1,
		TotalSize: int64(len(allData)),
	}
	var dataChunks []chunking.DataChunk
	chunkSize := mtc.ChunkSize
	if chunkSize == 0 {
		chunkSize = 1024 // Default
	}

	for i := 0; i < len(allData); i += chunkSize {
		end := i + chunkSize
		if end > len(allData) {
			end = len(allData)
		}
		chunkData := allData[i:end]
		hash := sha256.Sum256(chunkData)
		chunkCID := hex.EncodeToString(hash[:])
		dc := chunking.DataChunk{ChunkCID: chunkCID, Data: chunkData, Size: int64(len(chunkData))}
		dataChunks = append(dataChunks, dc)
		manifest.Chunks = append(manifest.Chunks, chunking.ChunkInfo{ChunkCID: chunkCID, Size: int64(len(chunkData))})
	}

	if mtc.ExpectedChunks > 0 && len(dataChunks) != mtc.ExpectedChunks {
		return nil, nil, fmt.Errorf("mock chunker expected %d chunks, got %d", mtc.ExpectedChunks, len(dataChunks))
	}

	var cidBuffer bytes.Buffer
	for _, ci := range manifest.Chunks {
		cidBuffer.WriteString(ci.ChunkCID)
	}
	manifestHash := sha256.Sum256(cidBuffer.Bytes())
	manifest.ManifestCID = "test_manifest_" + hex.EncodeToString(manifestHash[:]) // Prefix for clarity in tests
	manifest.EncryptionMethod = "none"

	return manifest, dataChunks, nil
}

// MockTestStorage is a mock for DDSStorage interface used in tests.
type MockTestStorage struct {
	mu            sync.Mutex
	chunks        map[string][]byte
	SimulateError bool
	StoreCount    int
}

func NewMockTestStorage() *MockTestStorage {
	return &MockTestStorage{chunks: make(map[string][]byte)}
}

func (mts *MockTestStorage) StoreChunk(chunkID string, data []byte) error {
	mts.mu.Lock()
	defer mts.mu.Unlock()
	if mts.SimulateError {
		return fmt.Errorf("mock storage simulated store error")
	}
	if chunkID == "" {
		return fmt.Errorf("chunkID cannot be empty")
	}
	mts.chunks[chunkID] = bytes.Clone(data)
	mts.StoreCount++
	return nil
}

func (mts *MockTestStorage) RetrieveChunk(chunkID string) ([]byte, error) {
	mts.mu.Lock()
	defer mts.mu.Unlock()
	data, ok := mts.chunks[chunkID]
	if !ok {
		return nil, fmt.Errorf("chunk %s not found", chunkID)
	}
	return bytes.Clone(data), nil
}

func (mts *MockTestStorage) ChunkExists(chunkID string) bool {
	mts.mu.Lock()
	defer mts.mu.Unlock()
	_, ok := mts.chunks[chunkID]
	return ok
}

// MockTestOriginator is a mock for OriginatorAdvertiser.
type MockTestOriginator struct {
	SimulateError   bool
	AdvertisedCID   string
	AdvertiseCount  int
}
func (mto *MockTestOriginator) AdvertiseManifest(manifest *chunking.ContentManifestV1) error {
	mto.AdvertiseCount++
	if mto.SimulateError {
		return fmt.Errorf("mock originator simulated advertise error")
	}
	if manifest != nil {
		mto.AdvertisedCID = manifest.ManifestCID
	}
	return nil
}


func TestContentPublisher_PublishTextPostToDDS(t *testing.T) {
	mockChunker := &MockTestChunker{ChunkSize: 64} // Small chunk size for easy testing
	mockStorage := NewMockTestStorage()
	mockOriginator := &MockTestOriginator{}

	publisher, err := NewContentPublisher(mockChunker, mockStorage, mockOriginator)
	if err != nil {
		t.Fatalf("NewContentPublisher() error = %v", err)
	}

	tests := []struct {
		name          string
		text          string
		chunkerError  bool
		storageError  bool
		originatorError bool
		wantErr       bool
		expectedChunks int
	}{
		{
			name: "valid text post",
			text: "This is a test post to be published to DDS.",
			expectedChunks: 1, // (45 bytes / 64 chunksize)
			wantErr: false,
		},
		{
			name: "longer text post (multiple chunks)",
			text: "This is a much longer test post that should definitely be split into multiple distinct chunks by our mock chunker setup with a small chunk size.",
			expectedChunks: 2, // (139 bytes / 64 chunksize)
			wantErr: false,
		},
		{
			name: "empty text post",
			text: "",
			wantErr: true,
		},
		{
			name: "chunker error",
			text: "Some text",
			chunkerError: true,
			wantErr: true,
		},
		{
			name: "storage error",
			text: "Some text that chunks fine",
			storageError: true,
			expectedChunks: 1,
			wantErr: true,
		},
		{
			name: "originator error (should still succeed if storage works)",
			text: "Text with originator error",
			originatorError: true,
			expectedChunks: 1,
			wantErr: false, // PublishTextPostToDDS currently only logs originator error
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mockChunker.SimulateError = tt.chunkerError
			mockStorage.SimulateError = tt.storageError
			mockStorage.StoreCount = 0 // Reset store count for each test
			mockOriginator.SimulateError = tt.originatorError
			mockOriginator.AdvertiseCount = 0
			mockOriginator.AdvertisedCID = ""
			mockChunker.ExpectedChunks = tt.expectedChunks


			cid, err := publisher.PublishTextPostToDDS(tt.text)
			if (err != nil) != tt.wantErr {
				t.Errorf("PublishTextPostToDDS() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !tt.wantErr {
				if cid == "" {
					t.Errorf("PublishTextPostToDDS() returned empty CID for successful case")
				}
				if mockStorage.StoreCount != tt.expectedChunks && tt.text != "" {
					t.Errorf("Expected %d chunks to be stored, got %d", tt.expectedChunks, mockStorage.StoreCount)
				}
				if mockOriginator.AdvertiseCount != 1 && tt.text != "" {
					t.Errorf("Expected AdvertiseManifest to be called once, called %d times", mockOriginator.AdvertiseCount)
				}
				if mockOriginator.AdvertisedCID != cid && tt.text != "" {
					t.Errorf("Originator advertised CID %s, but publisher returned %s", mockOriginator.AdvertisedCID, cid)
				}

				// Verify chunks were stored (simple check for existence of manifest's chunks)
				if tt.text != "" {
					manifestReader := bytes.NewReader([]byte(tt.text))
					manifest, dataChunks, _ := mockChunker.ChunkData(manifestReader) // Re-chunk to get expected CIDs
					if manifest != nil {
						for _, chunkInfo := range manifest.Chunks {
							if !mockStorage.ChunkExists(chunkInfo.ChunkCID) {
								t.Errorf("Expected chunk %s to be in mock storage, but it was not", chunkInfo.ChunkCID)
							}
						}
						// Check if the returned CID matches the manifest CID from re-chunking
						if cid != manifest.ManifestCID {
							t.Errorf("Returned CID %s does not match re-chunked manifest CID %s", cid, manifest.ManifestCID)
						}
					}
					if len(dataChunks) != mockStorage.StoreCount {
						t.Errorf("Number of data chunks (%d) from re-chunking does not match stored count (%d)", len(dataChunks), mockStorage.StoreCount)
					}
				}

			}
		})
	}
}

func TestNewContentPublisher_NilArgs(t *testing.T) {
	mockChunker := &MockTestChunker{}
	mockStorage := NewMockTestStorage()
	mockOriginator := &MockTestOriginator{}

	_, err := NewContentPublisher(nil, mockStorage, mockOriginator)
	if err == nil {
		t.Error("NewContentPublisher with nil chunker: expected error, got nil")
	}
	_, err = NewContentPublisher(mockChunker, nil, mockOriginator)
	if err == nil {
		t.Error("NewContentPublisher with nil storage: expected error, got nil")
	}
	_, err = NewContentPublisher(mockChunker, mockStorage, nil)
	if err == nil {
		t.Error("NewContentPublisher with nil originator: expected error, got nil")
	}
}
