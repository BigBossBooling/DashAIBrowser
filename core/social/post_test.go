package social

import (
	"encoding/json"
	"reflect"
	"testing"
	"time"
)

func TestNewPost(t *testing.T) {
	authorPK := "authorPubKey123"
	contentCID := "contentCID789"
	title := "My Test Post"
	tags := []string{"test", "go"}

	before := time.Now().UnixNano()
	post := NewPost(authorPK, contentCID, title, tags)
	after := time.Now().UnixNano()

	if post.AuthorPublicKey != authorPK {
		t.Errorf("NewPost AuthorPublicKey = %s, want %s", post.AuthorPublicKey, authorPK)
	}
	if post.ContentCID != contentCID {
		t.Errorf("NewPost ContentCID = %s, want %s", post.ContentCID, contentCID)
	}
	if post.Title != title {
		t.Errorf("NewPost Title = %s, want %s", post.Title, title)
	}
	if !reflect.DeepEqual(post.Tags, tags) {
		t.Errorf("NewPost Tags = %v, want %v", post.Tags, tags)
	}
	if post.Timestamp < before || post.Timestamp > after {
		t.Errorf("NewPost Timestamp %d is out of expected range (%d-%d)", post.Timestamp, before, after)
	}
	if post.Version != 1 {
		t.Errorf("NewPost Version = %d, want 1", post.Version)
	}
}

func TestPost_ToJSON_And_PostFromJSON(t *testing.T) {
	originalPost := NewPost("author456", "cidForJSONTest", "JSON Title", []string{"json", "serialization"})
	originalPost.Version = 3 // Set a specific version for test
	originalPost.Timestamp = time.Now().UnixNano() // Ensure specific timestamp

	jsonData, err := originalPost.ToJSON()
	if err != nil {
		t.Fatalf("Post.ToJSON() error = %v", err)
	}
	if len(jsonData) == 0 {
		t.Fatal("Post.ToJSON() returned empty data")
	}

	// Basic check: is it valid JSON?
	var temp map[string]interface{}
	if err := json.Unmarshal(jsonData, &temp); err != nil {
		t.Fatalf("Marshaled data is not valid JSON: %v\nData: %s", err, string(jsonData))
	}


	deserializedPost, err := PostFromJSON(jsonData)
	if err != nil {
		t.Fatalf("PostFromJSON() error = %v\nData: %s", err, string(jsonData))
	}

	if !reflect.DeepEqual(originalPost, deserializedPost) {
		t.Errorf("Deserialized post does not match original.\nOriginal: %+v\nDeserialized: %+v", originalPost, deserializedPost)
	}

	// Test error cases for PostFromJSON
	_, err = PostFromJSON([]byte("this is not json"))
	if err == nil {
		t.Error("PostFromJSON with invalid JSON: expected error, got nil")
	}
	_, err = PostFromJSON([]byte(`{"contentCID": "cid1", "timestamp": 123, "version": 1}`)) // Missing AuthorPublicKey
	if err == nil {
		t.Error("PostFromJSON with missing AuthorPublicKey: expected error, got nil")
	}
    _, err = PostFromJSON([]byte(`{"authorPublicKey": "pk1", "timestamp": 123, "version": 1}`)) // Missing ContentCID
	if err == nil {
		t.Error("PostFromJSON with missing ContentCID: expected error, got nil")
	}
    _, err = PostFromJSON([]byte(`{"authorPublicKey": "pk1", "contentCID": "cid1", "version": 1}`)) // Missing Timestamp
	if err == nil {
		t.Error("PostFromJSON with missing Timestamp: expected error, got nil")
	}
    _, err = PostFromJSON([]byte(`{"authorPublicKey": "pk1", "contentCID": "cid1", "timestamp": 123}`)) // Missing Version
	if err == nil {
		t.Error("PostFromJSON with missing Version: expected error, got nil")
	}
     _, err = PostFromJSON([]byte(`{"authorPublicKey": "pk1", "contentCID": "cid1", "timestamp": 123, "version": 0}`)) // Invalid Version
	if err == nil {
		t.Error("PostFromJSON with invalid version (0): expected error, got nil")
	}
}
