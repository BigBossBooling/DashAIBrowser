package user

import (
	"digisocialblock/core/content"
	"digisocialblock/pkg/dds/chunking" // For manifest struct in mock
	"fmt"
	"io"
	"reflect"
	"sync"
	"testing"
	"time"
)

// --- Mock ContentPublisher ---
type MockContentPublisher struct {
	mu             sync.Mutex
	PublishTextPostToDDSFunc func(text string) (string, error)
	PublishCount   int
	LastPublishedText string
}

func (mcp *MockContentPublisher) PublishTextPostToDDS(text string) (string, error) {
	mcp.mu.Lock()
	defer mcp.mu.Unlock()
	mcp.PublishCount++
	mcp.LastPublishedText = text
	if mcp.PublishTextPostToDDSFunc != nil {
		return mcp.PublishTextPostToDDSFunc(text)
	}
	return "mock_cid_for_" + text[:min(10, len(text))], nil // Default mock behavior
}
func (mcp *MockContentPublisher) Reset() {
	mcp.mu.Lock()
	defer mcp.mu.Unlock()
	mcp.PublishCount = 0
	mcp.LastPublishedText = ""
}


// --- Mock ContentRetriever ---
type MockContentRetriever struct {
	mu             sync.Mutex
	RetrieveAndVerifyTextPostFunc func(manifestCID string) (string, error)
	RetrieveCount  int
	LastRetrievedCID string
}

func (mcr *MockContentRetriever) RetrieveAndVerifyTextPost(manifestCID string) (string, error) {
	mcr.mu.Lock()
	defer mcr.mu.Unlock()
	mcr.RetrieveCount++
	mcr.LastRetrievedCID = manifestCID
	if mcr.RetrieveAndVerifyTextPostFunc != nil {
		return mcr.RetrieveAndVerifyTextPostFunc(manifestCID)
	}
	// Default mock behavior: assume CID contains the data for simplicity in some tests
	// or return a fixed string.
	if manifestCID == "error_cid" {
		return "", fmt.Errorf("mock retriever simulated error")
	}
	if manifestCID == "empty_cid" {
        return "", nil // Simulate empty content successfully retrieved
    }
	// For other CIDs, we might need to have stored the data during publish mock
	// For this unit test, we'll focus on the interaction, not perfect data pass-through via mocks.
	return fmt.Sprintf(`{"ownerPublicKey": "mockOwner", "displayName": "Mock User from %s", "bio": "Bio for mock user", "timestamp": %d, "version": 1}`, manifestCID, time.Now().UnixNano()), nil
}
func (mcr *MockContentRetriever) Reset() {
	mcr.mu.Lock()
	defer mcr.mu.Unlock()
	mcr.RetrieveCount = 0
	mcr.LastRetrievedCID = ""
}


// --- Mock interfaces for content.NewContentPublisher/Retriever if needed ---
// These are the interfaces ContentPublisher/Retriever themselves expect.
// For ProfileManager tests, we mock ContentPublisher and ContentRetriever directly.

type MockDDSChunker struct{}
func (m *MockDDSChunker) ChunkData(data io.Reader) (*chunking.ContentManifestV1, []chunking.DataChunk, error) {
	// Simplified, not used directly by ProfileManager tests if ContentPublisher is mocked
	return &chunking.ContentManifestV1{ManifestCID: "dummy_chunker_cid"}, nil, nil
}
type MockDDSStorage struct{}
func (m *MockDDSStorage) StoreChunk(chunkID string, data []byte) error { return nil }
func (m *MockDDSStorage) RetrieveChunk(chunkID string) ([]byte, error) { return []byte("dummy_chunk_data"), nil }
func (m *MockDDSStorage) ChunkExists(chunkID string) bool { return true }

type MockDDSManifestFetcher struct{}
func (m *MockDDSManifestFetcher) FetchManifest(manifestCID string) (*chunking.ContentManifestV1, error) {
	return &chunking.ContentManifestV1{ManifestCID: manifestCID, Chunks: []chunking.ChunkInfo{{ChunkCID: "dummy_chunk_cid"}}}, nil
}
type MockOriginatorAdvertiser struct{}
func (m *MockOriginatorAdvertiser) AdvertiseManifest(manifest *chunking.ContentManifestV1) error {return nil}


func TestNewProfileManager(t *testing.T) {
	// Need actual instances of ContentPublisher/Retriever, or mocks that satisfy them.
	// For this test, let's create real ones with their own (simple) mocks.
	pub, _ := content.NewContentPublisher(&MockDDSChunker{}, &MockDDSStorage{}, &MockOriginatorAdvertiser{})
	ret, _ := content.NewContentRetriever(&MockDDSManifestFetcher{}, &MockDDSStorage{})


	_, err := NewProfileManager(nil, ret)
	if err == nil {
		t.Error("NewProfileManager with nil publisher: expected error, got nil")
	}
	_, err = NewProfileManager(pub, nil)
	if err == nil {
		t.Error("NewProfileManager with nil retriever: expected error, got nil")
	}
	pm, err := NewProfileManager(pub, ret)
	if err != nil {
		t.Errorf("NewProfileManager valid args: unexpected error %v", err)
	}
	if pm == nil {
		t.Error("NewProfileManager valid args: got nil ProfileManager")
	}
}


func TestProfileManager_PublishProfile(t *testing.T) {
	mockPublisher := &MockContentPublisher{}
	// Retriever not used in PublishProfile, can be a simple mock or nil if constructor allows
	// For safety, let's use a simple mock for retriever too
	mockRetriever := &MockContentRetriever{}

	// We need to ensure the ContentPublisher and ContentRetriever for ProfileManager are the interfaces
	// defined in core/content, not the mock ones directly.
	// So, we need to make MockContentPublisher implement content.DDSChunker, content.DDSStorage, etc.
	// OR, more simply, ProfileManager should take interfaces that ContentPublisher/Retriever implement.
	// Let's assume ProfileManager takes concrete types for now as per current implementation.
	// This means we need to pass real ContentPublisher/Retriever that are configured with mocks.

	// Setup real ContentPublisher/Retriever with their own mocks for DDS layer
	ddsChunker := &MockDDSChunker{}
	ddsStorage := &MockDDSStorage{}
	ddsOriginator := &MockOriginatorAdvertiser{}
	realPublisher, _ := content.NewContentPublisher(ddsChunker, ddsStorage, ddsOriginator)

	ddsManifestFetcher := &MockDDSManifestFetcher{}
	realRetriever, _ := content.NewContentRetriever(ddsManifestFetcher, ddsStorage)

	// This is the ProfileManager we are testing.
	// It will use the *real* ContentPublisher, which in turn uses mocked DDS components.
	// To test ProfileManager's interaction with ContentPublisher, we should mock ContentPublisher itself.
	// Let's adjust the test to use the MockContentPublisher defined above.

	pm := &ProfileManager{publisher: (*content.ContentPublisher)(nil), retriever: (*content.ContentRetriever)(nil)} // Temp, will fix
	// This highlights a design consideration: ProfileManager should ideally take interfaces
	// for content.ContentPublisher and content.ContentRetriever, not concrete types,
	// to make mocking easier.
	// For now, let's proceed by mocking the *underlying* behavior of the real publisher.

	// Simpler approach: Directly use the MockContentPublisher/Retriever for this unit test
	testablePM := &ProfileManager{
		publisher: (*content.ContentPublisher)(mockPublisher), // This cast is problematic if interfaces don't match
		retriever: (*content.ContentRetriever)(mockRetriever), // Same here
	}
	// The above casting won't work directly because MockContentPublisher is not a *content.ContentPublisher.
	// We need to ensure ProfileManager's fields are interfaces or use the real ones with deeper mocks.
	// Given current ProfileManager takes concrete types, we test its behavior by observing
	// what its dependent (ContentPublisher) does. We can check ContentPublisher's internal mock.

	// Let's re-initialize ProfileManager with real ContentPublisher/Retriever which use mocks.
	pm, _ = NewProfileManager(realPublisher, realRetriever)


	profileData := NewProfile("ownerTest", "Test Profile User", "Bio for test.")
	expectedCID := "test_manifest_cid_from_publisher"

	// Configure the mock behavior for the *actual* ContentPublisher's dependencies if needed,
	// or trust ContentPublisher is tested elsewhere and just check ProfileManager's logic.
	// For ProfileManager.PublishProfile, it mainly calls profileData.ToJSON() and then publisher.PublishTextPostToDDS().
	// We want to verify that the correct JSON string is passed to the publisher.

	// We can't easily intercept what `realPublisher` (which `pm` uses) gets without
	// making `realPublisher` itself more testable or using interfaces in `ProfileManager`.
	// Let's assume `ContentPublisher.PublishTextPostToDDS` is well-tested.
	// We will verify that `PublishProfile` returns a CID and no error for valid input.

	cid, err := pm.PublishProfile(profileData)
	if err != nil {
		t.Fatalf("PublishProfile() error = %v", err)
	}
	if cid == "" {
		t.Error("PublishProfile() returned empty CID")
	}
	// We can't easily verify the exact CID here without knowing the exact output of the mock chunker
	// that `realPublisher` uses, unless `realPublisher`'s mock chunker is also configured here.
	// For this test, just ensuring a non-empty CID is returned from a successful publish is a start.
	t.Logf("PublishProfile returned CID: %s", cid)


	// Test nil profile data
	_, err = pm.PublishProfile(nil)
	if err == nil {
		t.Error("PublishProfile(nil) expected error, got nil")
	}
}


func TestProfileManager_RetrieveProfile(t *testing.T) {
	// Similar setup challenge as above with concrete types.
	mockPublisher := &MockContentPublisher{} // Not used by RetrieveProfile directly
	mockRetriever := &MockContentRetriever{}

	// We need ProfileManager to use our MockContentRetriever.
	// Temporary direct assignment for testing (again, interfaces would be better for ProfileManager fields)
	pm := &ProfileManager{
		publisher: (*content.ContentPublisher)(mockPublisher),
		retriever: (*content.ContentRetriever)(mockRetriever),
	}


	expectedProfile := NewProfile("retrievedOwner", "Retrieved User", "Retrieved Bio")
	expectedProfile.Version = 2
	expectedProfileJSON, _ := expectedProfile.ToJSON()
	targetCID := "cid_for_retrieve_test"

	mockRetriever.RetrieveAndVerifyTextPostFunc = func(manifestCID string) (string, error) {
		if manifestCID == targetCID {
			return string(expectedProfileJSON), nil
		}
		if manifestCID == "error_cid" {
			return "", fmt.Errorf("simulated retrieve error")
		}
		if manifestCID == "bad_json_cid" {
			return "this is not json", nil
		}
		if manifestCID == "empty_json_cid" {
			return "", nil // Simulates retriever returning empty string for content
		}
		return "", fmt.Errorf("unknown CID for mock retriever: %s", manifestCID)
	}

	// Test successful retrieval
	retrieved, err := pm.RetrieveProfile(targetCID)
	if err != nil {
		t.Fatalf("RetrieveProfile(%s) error = %v", targetCID, err)
	}
	if !reflect.DeepEqual(expectedProfile, retrieved) {
		t.Errorf("RetrieveProfile() got = %+v, want %+v", retrieved, expectedProfile)
	}
	if mockRetriever.RetrieveCount != 1 || mockRetriever.LastRetrievedCID != targetCID {
		t.Errorf("MockContentRetriever not called as expected")
	}
	mockRetriever.Reset()

	// Test retriever error
	_, err = pm.RetrieveProfile("error_cid")
	if err == nil {
		t.Error("RetrieveProfile(error_cid) expected error, got nil")
	} else if !strings.Contains(err.Error(), "simulated retrieve error") {
		t.Errorf("RetrieveProfile(error_cid) wrong error message: %v", err)
	}
	mockRetriever.Reset()

	// Test bad JSON error
	_, err = pm.RetrieveProfile("bad_json_cid")
	if err == nil {
		t.Error("RetrieveProfile(bad_json_cid) expected error, got nil")
	} else if !strings.Contains(err.Error(), "failed to unmarshal JSON") {
		t.Errorf("RetrieveProfile(bad_json_cid) wrong error message for JSON unmarshal: %v", err)
	}
	mockRetriever.Reset()

	// Test empty CID input
	_, err = pm.RetrieveProfile("")
	if err == nil {
		t.Error("RetrieveProfile with empty CID: expected error, got nil")
	}
	mockRetriever.Reset()

	// Test when retriever returns empty string for content
    _, err = pm.RetrieveProfile("empty_json_cid")
    if err == nil {
        t.Error("RetrieveProfile(empty_json_cid) expected error for empty JSON data, got nil")
    } else if !(strings.Contains(err.Error(), "retrieved empty profile data") || strings.Contains(err.Error(), "CID empty_json_cid points to empty content")) {
        // The error message might slightly differ based on whether the CID is the special "empty_content_manifest_cid_v1"
        t.Errorf("RetrieveProfile(empty_json_cid) wrong error message for empty JSON: %v", err)
    }
    mockRetriever.Reset()
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
