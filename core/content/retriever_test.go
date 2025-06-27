package content

import (
	"bytes"
	"crypto/sha256"
	"digisocialblock/pkg/dds/chunking" // Assuming this path
	"encoding/hex"
	"fmt"
	"io"
	"strings"
	"sync"
	"testing"
)

// --- Mock Implementations for Retriever Tests ---

// MockTestManifestFetcher allows controlling the manifest returned for a CID.
type MockTestManifestFetcher struct {
	mu         sync.Mutex
	manifests  map[string]*chunking.ContentManifestV1
	SimulateError bool
	ErrorToReturn error
}

func NewMockTestManifestFetcher() *MockTestManifestFetcher {
	return &MockTestManifestFetcher{manifests: make(map[string]*chunking.ContentManifestV1)}
}

func (mf *MockTestManifestFetcher) AddManifest(cid string, manifest *chunking.ContentManifestV1) {
	mf.mu.Lock()
	defer mf.mu.Unlock()
	mf.manifests[cid] = manifest
}

func (mf *MockTestManifestFetcher) FetchManifest(manifestCID string) (*chunking.ContentManifestV1, error) {
	mf.mu.Lock()
	defer mf.mu.Unlock()
	if mf.SimulateError {
		return nil, mf.ErrorToReturn
	}
	m, ok := mf.manifests[manifestCID]
	if !ok {
		return nil, fmt.Errorf("test manifest fetcher: manifest %s not found", manifestCID)
	}
	// Return a copy
	mCopy := *m
	mCopy.Chunks = make([]chunking.ChunkInfo, len(m.Chunks))
	copy(mCopy.Chunks, m.Chunks)
	return &mCopy, nil
}

// MockTestChunkRetriever allows controlling chunks returned for a CID.
// Reuses MockTestStorage from publisher_test.go as it fits the DDSChunkRetriever interface.
// For clarity, we can alias it or use it directly. Let's use MockTestStorage.
// type MockTestChunkRetriever MockTestStorage (if methods match directly)
// For this test, let's define one specific to retriever tests for more control over errors.

type ControlledMockChunkRetriever struct {
	mu            sync.Mutex
	chunks        map[string][]byte
	SimulateErrorOnCID string
	ErrorToReturn      error
	CorruptChunkCID    string // If set, data for this CID will be altered
}

func NewControlledMockChunkRetriever() *ControlledMockChunkRetriever {
	return &ControlledMockChunkRetriever{chunks: make(map[string][]byte)}
}

func (cr *ControlledMockChunkRetriever) AddChunk(cid string, data []byte) {
	cr.mu.Lock()
	defer cr.mu.Unlock()
	cr.chunks[cid] = data
}

func (cr *ControlledMockChunkRetriever) RetrieveChunk(chunkCID string) ([]byte, error) {
	cr.mu.Lock()
	defer cr.mu.Unlock()
	if cr.SimulateErrorOnCID == chunkCID {
		return nil, cr.ErrorToReturn
	}
	data, ok := cr.chunks[chunkCID]
	if !ok {
		return nil, fmt.Errorf("test chunk retriever: chunk %s not found", chunkCID)
	}
	if cr.CorruptChunkCID == chunkCID {
		corruptedData := bytes.Clone(data)
		if len(corruptedData) > 0 {
			corruptedData[0] = corruptedData[0] ^ 0xff // Flip a bit
		} else { // Handle empty chunk case if it needs specific corruption
			corruptedData = []byte("corrupted_empty_chunk_data")
		}
		return corruptedData, nil
	}
	return bytes.Clone(data), nil
}

func (cr *ControlledMockChunkRetriever) ChunkExists(chunkCID string) bool {
	cr.mu.Lock()
	defer cr.mu.Unlock()
	_, ok := cr.chunks[chunkCID]
	return ok
}


// --- Helper to create sample manifest and chunks for tests ---
func createSampleContentAndManifest(text string, chunkSize int) (string, *chunking.ContentManifestV1, map[string][]byte) {
	mockChunker := &MockTestChunker{ChunkSize: chunkSize} // Using the one from publisher_test for consistency
	reader := strings.NewReader(text)
	manifest, dataChunks, _ := mockChunker.ChunkData(reader)

	storedChunks := make(map[string][]byte)
	for _, dc := range dataChunks {
		storedChunks[dc.ChunkCID] = dc.Data
	}
	return manifest.ManifestCID, manifest, storedChunks
}


func TestContentRetriever_RetrieveAndVerifyTextPost(t *testing.T) {
	sampleText := "This is a sample text that will be chunked and retrieved. It needs to be long enough for multiple chunks with a small chunk size."
	chunkSize := 32 // Small chunk size for testing

	expectedManifestCID, expectedManifest, expectedChunksMap := createSampleContentAndManifest(sampleText, chunkSize)

	tests := []struct {
		name                string
		manifestCIDToFetch  string
		setupFetcher        func(*MockTestManifestFetcher)
		setupChunkRetriever func(*ControlledMockChunkRetriever)
		wantErrMsgContains  string // Substring of expected error, empty if no error
	}{
		{
			name:               "successful retrieval",
			manifestCIDToFetch: expectedManifestCID,
			setupFetcher: func(mf *MockTestManifestFetcher) {
				mf.AddManifest(expectedManifestCID, expectedManifest)
			},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {
				for cid, data := range expectedChunksMap {
					cr.AddChunk(cid, data)
				}
			},
			wantErrMsgContains: "",
		},
		{
			name:               "manifest fetch error",
			manifestCIDToFetch: "nonexistent_cid",
			setupFetcher: func(mf *MockTestManifestFetcher) {
				mf.SimulateError = true
				mf.ErrorToReturn = fmt.Errorf("simulated manifest fetch network error")
			},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {},
			wantErrMsgContains:  "simulated manifest fetch network error",
		},
		{
			name:               "manifest not found",
			manifestCIDToFetch: "cid_that_fetcher_does_not_have",
			setupFetcher:       func(mf *MockTestManifestFetcher) { /* No manifest added */ },
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {},
			wantErrMsgContains:  "manifest CID cid_that_fetcher_does_not_have not found",
		},
		{
			name:               "chunk retrieval error",
			manifestCIDToFetch: expectedManifestCID,
			setupFetcher: func(mf *MockTestManifestFetcher) {
				mf.AddManifest(expectedManifestCID, expectedManifest)
			},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {
				// Store all but one chunk
				i := 0
				for cid, data := range expectedChunksMap {
					if i < len(expectedChunksMap)-1 { // Skip last chunk
						cr.AddChunk(cid, data)
					}
					i++
				}
				if len(expectedManifest.Chunks) > 0 {
					lastChunkCID := expectedManifest.Chunks[len(expectedManifest.Chunks)-1].ChunkCID
					cr.SimulateErrorOnCID = lastChunkCID
					cr.ErrorToReturn = fmt.Errorf("simulated chunk retrieve error")
				}
			},
			wantErrMsgContains: "simulated chunk retrieve error",
		},
		{
			name:               "chunk not found in storage",
			manifestCIDToFetch: expectedManifestCID,
			setupFetcher: func(mf *MockTestManifestFetcher) {
				mf.AddManifest(expectedManifestCID, expectedManifest)
			},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {
				// Store all but one chunk, and don't simulate error, just let it be missing
				i := 0
				for cid, data := range expectedChunksMap {
					if i < len(expectedChunksMap)-1 {
						cr.AddChunk(cid, data)
					}
					i++
				}
			},
			wantErrMsgContains: "not found in storage", // Error from ChunkExists being false
		},
		{
			name:               "chunk integrity verification failure (corrupted chunk)",
			manifestCIDToFetch: expectedManifestCID,
			setupFetcher: func(mf *MockTestManifestFetcher) {
				mf.AddManifest(expectedManifestCID, expectedManifest)
			},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {
				for cid, data := range expectedChunksMap {
					cr.AddChunk(cid, data)
				}
				if len(expectedManifest.Chunks) > 0 {
					cr.CorruptChunkCID = expectedManifest.Chunks[0].ChunkCID
				}
			},
			wantErrMsgContains: "integrity check failed for chunk",
		},
		{
			name:               "total size mismatch",
			manifestCIDToFetch: expectedManifestCID,
			setupFetcher: func(mf *MockTestManifestFetcher) {
				corruptedManifest := *expectedManifest // copy
				corruptedManifest.TotalSize = expectedManifest.TotalSize + 10
				mf.AddManifest(expectedManifestCID, &corruptedManifest)
			},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {
				for cid, data := range expectedChunksMap {
					cr.AddChunk(cid, data)
				}
			},
			wantErrMsgContains: "reassembled content size mismatch",
		},
		{
			name:               "empty manifest CID",
			manifestCIDToFetch: "",
			setupFetcher:       func(mf *MockTestManifestFetcher) {},
			setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {},
			wantErrMsgContains:  "manifest CID cannot be empty",
		},
		{
            name: "successful retrieval of empty content",
            manifestCIDToFetch: "empty_content_manifest_cid_v1", // From MockChunker logic
            setupFetcher: func(mf *MockTestManifestFetcher) {
				// MockChunker used in createSampleContentAndManifest would store this if "" was passed
				// For this test, we explicitly add an empty manifest.
                emptyManifest := &chunking.ContentManifestV1{
                    ManifestCID: "empty_content_manifest_cid_v1",
                    TotalSize: 0,
                    Chunks: []chunking.ChunkInfo{},
                }
                mf.AddManifest("empty_content_manifest_cid_v1", emptyManifest)
            },
            setupChunkRetriever: func(cr *ControlledMockChunkRetriever) {},
            wantErrMsgContains:  "", // Expect no error, empty string result
        },

	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mockFetcher := NewMockTestManifestFetcher()
			mockRetriever := NewControlledMockChunkRetriever()

			tt.setupFetcher(mockFetcher)
			tt.setupChunkRetriever(mockRetriever)

			cr, err := NewContentRetriever(mockFetcher, mockRetriever)
			if err != nil {
				t.Fatalf("NewContentRetriever() failed: %v", err)
			}

			retrievedText, err := cr.RetrieveAndVerifyTextPost(tt.manifestCIDToFetch)

			if tt.wantErrMsgContains != "" {
				if err == nil {
					t.Errorf("RetrieveAndVerifyTextPost() expected error containing '%s', but got nil error", tt.wantErrMsgContains)
				} else if !strings.Contains(err.Error(), tt.wantErrMsgContains) {
					t.Errorf("RetrieveAndVerifyTextPost() error = '%v', expected to contain '%s'", err, tt.wantErrMsgContains)
				}
			} else {
				if err != nil {
					t.Errorf("RetrieveAndVerifyTextPost() unexpected error = %v", err)
				}
				expectedText := sampleText
				if tt.manifestCIDToFetch == "empty_content_manifest_cid_v1" {
					expectedText = ""
				}
				if retrievedText != expectedText {
					t.Errorf("RetrieveAndVerifyTextPost() retrieved text = '%s', want '%s'", retrievedText, expectedText)
				}
			}
		})
	}
}
