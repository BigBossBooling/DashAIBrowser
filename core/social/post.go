package social

import (
	"encoding/json"
	"fmt"
	"time"
)

// Post represents the metadata of a user's post.
// The actual content of the post is stored on DDS and referenced by ContentCID.
type Post struct {
	AuthorPublicKey string   `json:"authorPublicKey"` // Hex-encoded public key of the post author
	ContentCID      string   `json:"contentCID"`      // CID of the post content stored on DDS
	Timestamp       int64    `json:"timestamp"`       // UnixNano timestamp of when the post was created (or this version)
	Version         int      `json:"version"`         // Version of the post (for edits)
	Title           string   `json:"title,omitempty"` // Optional title for the post
	Tags            []string `json:"tags,omitempty"`  // Optional tags
	// ReplyToPostCID  string   `json:"replyToPostCID,omitempty"` // If this post is a reply to another
	// RepostOfPostCID string   `json:"repostOfPostCID,omitempty"`// If this is a repost
}

// NewPost creates a new Post metadata instance.
// authorPublicKey is the hex-encoded public key string.
// contentCID is the CID of the post's actual content on DDS.
func NewPost(authorPublicKey, contentCID, title string, tags []string) *Post {
	return &Post{
		AuthorPublicKey: authorPublicKey,
		ContentCID:      contentCID,
		Timestamp:       time.Now().UnixNano(),
		Version:         1, // Initial version
		Title:           title,
		Tags:            tags,
	}
}

// ToJSON serializes the Post struct to a JSON byte slice.
func (p *Post) ToJSON() ([]byte, error) {
	jsonData, err := json.MarshalIndent(p, "", "  ")
	if err != nil {
		return nil, fmt.Errorf("failed to marshal post to JSON: %w", err)
	}
	return jsonData, nil
}

// PostFromJSON deserializes a JSON byte slice into a Post struct.
func PostFromJSON(jsonData []byte) (*Post, error) {
	var p Post
	err := json.Unmarshal(jsonData, &p)
	if err != nil {
		return nil, fmt.Errorf("failed to unmarshal JSON to post: %w", err)
	}
	// Basic validation
	if p.AuthorPublicKey == "" {
		return nil, fmt.Errorf("unmarshaled post has empty AuthorPublicKey")
	}
	if p.ContentCID == "" {
		return nil, fmt.Errorf("unmarshaled post has empty ContentCID")
	}
	if p.Timestamp == 0 {
		return nil, fmt.Errorf("unmarshaled post has zero timestamp")
	}
	if p.Version <= 0 {
		return nil, fmt.Errorf("unmarshaled post has invalid version: %d", p.Version)
	}
	return &p, nil
}
