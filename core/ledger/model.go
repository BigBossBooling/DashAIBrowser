package ledger

import "time"

// TransactionType defines the type of action a transaction represents.
type TransactionType string

const (
	PostCreated   TransactionType = "PostCreated"
	CommentAdded  TransactionType = "CommentAdded"
	Like          TransactionType = "Like"
	UserFollowed  TransactionType = "UserFollowed"
	ProfileUpdate TransactionType = "ProfileUpdate"
	// Add other transaction types as needed
)

// Transaction represents a single action or event in the Digisocialblock system.
type Transaction struct {
	ID              string          `json:"id"`              // Unique identifier (hash of key transaction data)
	Timestamp       int64           `json:"timestamp"`       // Unix timestamp of when the transaction was created
	SenderPublicKey string          `json:"senderPublicKey"` // Public key of the user initiating the transaction
	Type            TransactionType `json:"type"`            // Type of the transaction (e.g., "PostCreated")
	Payload         []byte          `json:"payload"`         // Serialized data specific to the transaction type (e.g., post content CID, comment details)
	Signature       []byte          `json:"signature"`       // Cryptographic signature of the transaction data
}

// Block represents a collection of transactions, forming a unit in the blockchain.
type Block struct {
	Index         int64          `json:"index"`         // Position of the block in the chain
	Timestamp     int64          `json:"timestamp"`     // Unix timestamp of when the block was created/mined
	Transactions  []*Transaction `json:"transactions"`  // List of transactions included in this block
	PrevBlockHash string         `json:"prevBlockHash"` // Hash of the previous block in the chain
	Hash          string         `json:"hash"`          // Cryptographic hash of this block's content (excluding this Hash field itself)
	// Nonce int64 `json:"nonce"` // Optional: For Proof-of-Work or other consensus mechanisms
}

// Helper method to create a human-readable representation of a Transaction (optional).
// func (t *Transaction) String() string {
//  return fmt.Sprintf("Transaction:\n  ID: %s\n  Timestamp: %s\n  Sender: %s\n  Type: %s\n  Payload: %s\n  Signature: %x\n",
//      t.ID, time.Unix(t.Timestamp, 0).Format(time.RFC3339), t.SenderPublicKey, t.Type, string(t.Payload), t.Signature)
// }

// Helper method to create a human-readable representation of a Block (optional).
// func (b *Block) String() string {
//  var txIDs []string
//  for _, tx := range b.Transactions {
//      txIDs = append(txIDs, tx.ID)
//  }
//  return fmt.Sprintf("Block:\n  Index: %d\n  Timestamp: %s\n  PrevHash: %s\n  Hash: %s\n  Transactions: %v\n",
//      b.Index, time.Unix(b.Timestamp, 0).Format(time.RFC3339), b.PrevBlockHash, b.Hash, txIDs)
// }

// Note: BlockHeaderForHashing and TransactionForHashing structs were removed as
// the deterministic string generation functions in crypto_utils.go take primitive types directly,
// making these specific intermediate structs for hashing less necessary.
// If more complex serialization or a different hashing strategy (e.g., gob encoding directly from structs)
// were used, they might be reintroduced or adapted.
