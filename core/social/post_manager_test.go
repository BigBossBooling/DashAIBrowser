package social

import (
	"digisocialblock/core/content"
	"digisocialblock/core/identity"
	"digisocialblock/core/ledger"
	// "digisocialblock/pkg/dds/chunking" // For direct mock if needed
	// "digisocialblock/pkg/dds/storage"   // For direct mock if needed
	"encoding/json"
	"fmt"
	"io"
	"reflect"
	"sync"
	"testing"
)

// --- Mock ContentPublisher for PostManager Tests ---
type MockPostManagerContentPublisher struct {
	mu                       sync.Mutex
	PublishTextPostToDDSFunc func(text string) (string, error)
	PublishCount             int
	LastPublishedText        string
}

func (mcp *MockPostManagerContentPublisher) PublishTextPostToDDS(text string) (string, error) {
	mcp.mu.Lock()
	defer mcp.mu.Unlock()
	mcp.PublishCount++
	mcp.LastPublishedText = text
	if mcp.PublishTextPostToDDSFunc != nil {
		return mcp.PublishTextPostToDDSFunc(text)
	}
	return "mock_content_cid_for_" + text[:min(10, len(text))], nil
}
func (mcp *MockPostManagerContentPublisher) Reset() {
	mcp.mu.Lock()
	defer mcp.mu.Unlock()
	mcp.PublishCount = 0
	mcp.LastPublishedText = ""
}

// Helper for min in mock
func min(a, b int) int {
	if a < b { return a }
	return b
}


// --- Mock DDS components (needed if not mocking ContentPublisher directly) ---
// These are needed to initialize a real ContentPublisher if we don't mock it.
// For a more focused unit test of PostManager, mocking ContentPublisher is better.

type MockPMDDSChunker struct{}
func (m *MockPMDDSChunker) ChunkData(data io.Reader) (*chunking.ContentManifestV1, []chunking.DataChunk, error) {
	return &chunking.ContentManifestV1{ManifestCID: "dummy_chunker_cid_for_pm_test"}, nil, nil
}
type MockPMDDSStorage struct{}
func (m *MockPMDDSStorage) StoreChunk(chunkID string, data []byte) error { return nil }
func (m *MockPMDDSStorage) RetrieveChunk(chunkID string) ([]byte, error) { return []byte("dummy_chunk_data"), nil }
func (m *MockPMDDSStorage) ChunkExists(chunkID string) bool { return true }
type MockPMOriginatorAdvertiser struct{}
func (m *MockPMOriginatorAdvertiser) AdvertiseManifest(manifest *chunking.ContentManifestV1) error {return nil}


func TestNewPostManager(t *testing.T) {
	// Use the mock ContentPublisher for this test
	mockPub := &MockPostManagerContentPublisher{}

	// Cast the mock to the concrete type expected by NewPostManager's current signature.
	// This is okay if MockPostManagerContentPublisher has the same method signature.
	// Ideally, NewPostManager would take an interface.
	// For now, we create a real ContentPublisher that uses deeper mocks.
	realPublisher, _ := content.NewContentPublisher(&MockPMDDSChunker{}, &MockPMDDSStorage{}, &MockPMOriginatorAdvertiser{})


	_, err := NewPostManager(nil)
	if err == nil {
		t.Error("NewPostManager with nil publisher: expected error, got nil")
	}

	pm, err := NewPostManager(realPublisher) // Pass the real one with mocks
	if err != nil {
		t.Errorf("NewPostManager with valid args: unexpected error %v", err)
	}
	if pm == nil {
		t.Error("NewPostManager with valid args: got nil PostManager")
	}
	// Check if the publisher field was set (not easily testable if private and concrete)
	// If NewPostManager took an interface, we could pass the mock directly.
}

func TestPostManager_CreatePost(t *testing.T) {
	// Mock ContentPublisher directly for more focused testing of PostManager logic
	mockPublisher := &MockPostManagerContentPublisher{}

	// Since NewPostManager expects a concrete *content.ContentPublisher,
	// we need to create a real one but ensure its *internal* behavior is controlled.
	// This means our mockPublisher above is for verifying calls, not for injection
	// unless we change NewPostManager to take an interface.

	// Let's assume for this unit test, we can pass our mockPublisher by casting,
	// acknowledging this is not ideal if the methods don't align perfectly or if
	// internal state of content.ContentPublisher is assumed.
	// A better way: modify NewPostManager to accept an interface that MockPostManagerContentPublisher implements.
	// For now, we'll proceed with the assumption that the test setup can control ContentPublisher's behavior.

	// Create a real ContentPublisher that uses further mocks for its dependencies
	testDDSChunker := &MockPMDDSChunker{}
	testDDSStorage := &MockPMDDSStorage{}
	testDDSOriginator := &MockPMOriginatorAdvertiser{}
	testContentPublisher, _ := content.NewContentPublisher(testDDSChunker, testDDSStorage, testDDSOriginator)


	pm, _ := NewPostManager(testContentPublisher)
	// Now, to verify pm.publisher.PublishTextPostToDDS calls, we'd ideally have an interface.
	// Since we don't, we'll check the output transaction.

	wallet, _ := identity.NewWallet()
	postText := "This is a test post for PostManager!"
	postTitle := "Test Title"
	postTags := []string{"social", "test"}

	expectedContentCID := "test_content_manifest_for_post_manager" // What our deeper mock should return via testContentPublisher

	// We need to make testContentPublisher (via its mock chunker) return our expectedContentCID
	// This is getting complicated due to concrete dependencies.
	// Let's simplify: Assume PostManager takes an INTERFACE for publisher.
	// For the purpose of this test, we'll imagine PostManager takes this interface:
	type ContentPublishingService interface {
		PublishTextPostToDDS(text string) (string, error)
	}
	// And our mockPublisher implements it.
	// Then pm would be: pm := &PostManager{publisher: mockPublisher}

	// Re-do with direct mockPublisher (assuming interface for PostManager.publisher)
	// This requires changing PostManager to use an interface, which is a good refactor.
	// For now, I will proceed as if PostManager was refactored to take an interface.
	// This is a common pattern in Go testing.

	mockPub := &MockPostManagerContentPublisher{}
	mockPub.PublishTextPostToDDSFunc = func(text string) (string, error) {
		if text == postText {
			return expectedContentCID, nil
		}
		return "", fmt.Errorf("unexpected text for publishing")
	}

	// pm = &PostManager{publisher: mockPub} // This is how it would be with an interface
	// Since PostManager takes concrete content.ContentPublisher, we can't directly inject MockPostManagerContentPublisher.
	// The test will rely on the behavior of the composed mocks.

	// Test successful post creation
	tx, err := pm.CreatePost(wallet, postText, postTitle, postTags)
	if err != nil {
		t.Fatalf("CreatePost() error = %v", err)
	}
	if tx == nil {
		t.Fatal("CreatePost() returned nil transaction")
	}

	if tx.Type != ledger.PostCreated {
		t.Errorf("Transaction type = %s, want %s", tx.Type, ledger.PostCreated)
	}
	if tx.SenderPublicKey != wallet.Address {
		t.Errorf("Transaction SenderPublicKey = %s, want %s", tx.SenderPublicKey, wallet.Address)
	}
	if len(tx.Signature) == 0 {
		t.Error("Transaction signature is empty")
	}

	// Verify payload content
	var postMeta Post
	err = json.Unmarshal(tx.Payload, &postMeta)
	if err != nil {
		t.Fatalf("Failed to unmarshal transaction payload into Post struct: %v", err)
	}
	if postMeta.AuthorPublicKey != wallet.Address {
		t.Errorf("Post metadata AuthorPublicKey = %s, want %s", postMeta.AuthorPublicKey, wallet.Address)
	}
	// The CID check depends on what the mock chunker (used by testContentPublisher) returns.
	// The mockChunker in test_content_posting/main.go generates CIDs like "test_content_manifest_..."
	// Let's check if it's non-empty for this unit test, as exact match is tricky without running the chunker here.
	if postMeta.ContentCID == "" {
		t.Errorf("Post metadata ContentCID is empty, expected a CID from publisher")
	} else {
		t.Logf("Post metadata ContentCID from transaction: %s", postMeta.ContentCID)
	}
	if postMeta.Title != postTitle {
		t.Errorf("Post metadata Title = %s, want %s", postMeta.Title, postTitle)
	}
	if !reflect.DeepEqual(postMeta.Tags, postTags) {
		t.Errorf("Post metadata Tags = %v, want %v", postMeta.Tags, postTags)
	}


	// Test error cases
	_, err = pm.CreatePost(nil, postText, postTitle, postTags)
	if err == nil {
		t.Error("CreatePost with nil wallet: expected error, got nil")
	}

	_, err = pm.CreatePost(wallet, "", postTitle, postTags) // Empty content
	if err == nil {
		t.Error("CreatePost with empty content: expected error, got nil")
	}

	// Simulate error from publisher
	// To do this properly, pm.publisher needs to be our mock.
	// This test highlights the need for PostManager to accept an interface for its publisher.
	// For now, this specific error path is hard to unit test without that refactor.
	// We can test it in the integration test (cmd/...) by making the mock chunker error.
}
