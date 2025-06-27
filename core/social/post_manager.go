package social

import (
	"digisocialblock/core/content"
	"digisocialblock/core/identity"
	"digisocialblock/core/ledger"
	"fmt"
)

// PostManager handles the business logic for creating and managing posts.
type PostManager struct {
	publisher *content.ContentPublisher
	// Potentially a ContentRetriever if PostManager also handles fetching post content details
	// For now, focusing on creation.
}

// NewPostManager creates a new PostManager.
func NewPostManager(publisher *content.ContentPublisher) (*PostManager, error) {
	if publisher == nil {
		return nil, fmt.Errorf("content publisher cannot be nil for PostManager")
	}
	return &PostManager{
		publisher: publisher,
	}, nil
}

// CreatePost handles the full process of creating a user post:
// 1. Publishes the raw text content to DDS to get a ContentCID.
// 2. Creates Post metadata (including AuthorPublicKey and ContentCID).
// 3. Serializes the Post metadata to JSON to be used as transaction payload.
// 4. Creates a new ledger.Transaction of type "PostCreated".
// 5. Signs the transaction using the user's wallet.
// Returns the signed ledger.Transaction, ready to be added to the blockchain.
func (pm *PostManager) CreatePost(
	wallet *identity.Wallet,
	rawTextContent string,
	title string, // Optional title
	tags []string, // Optional tags
) (*ledger.Transaction, error) {
	if wallet == nil {
		return nil, fmt.Errorf("wallet cannot be nil to create a post")
	}
	if rawTextContent == "" {
		// Depending on rules, empty content might be allowed if title/tags are primary.
		// For now, let's assume rawTextContent is the primary content.
		return nil, fmt.Errorf("raw text content cannot be empty for a post")
	}

	// 1. Publish raw text content to DDS
	contentCID, err := pm.publisher.PublishTextPostToDDS(rawTextContent)
	if err != nil {
		return nil, fmt.Errorf("failed to publish post content to DDS: %w", err)
	}
	if contentCID == "" {
		return nil, fmt.Errorf("DDS publisher returned an empty content CID")
	}

	// 2. Create Post metadata struct
	postMeta := NewPost(wallet.Address, contentCID, title, tags)

	// 3. Serialize Post metadata to JSON for the transaction payload
	postPayloadJSON, err := postMeta.ToJSON()
	if err != nil {
		return nil, fmt.Errorf("failed to serialize post metadata to JSON: %w", err)
	}

	// 4. Create a new ledger.Transaction
	tx, err := ledger.NewTransaction(wallet.Address, ledger.PostCreated, postPayloadJSON)
	if err != nil {
		return nil, fmt.Errorf("failed to create new ledger transaction for post: %w", err)
	}

	// 5. Sign the transaction using the wallet
	err = wallet.SignTransaction(tx) // This method is on the Wallet struct
	if err != nil {
		return nil, fmt.Errorf("failed to sign post transaction: %w", err)
	}

	return tx, nil
}

// TODO: Future methods for PostManager:
// - GetPostByCID(cid string) (*Post, string_content, error) // Needs ContentRetriever
// - UpdatePost(...)
// - DeletePost(...) // Complex in immutable systems; might mean creating a "deleted" marker.
// - ListPostsByUser(authorPublicKey string) ([]*Post, error) // Needs indexing.
