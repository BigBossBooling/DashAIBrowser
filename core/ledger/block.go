package ledger

import (
	"fmt"
	"time"
)

// NewBlock creates and returns a new block in the blockchain.
// It takes the index, the hash of the previous block, and a list of transactions.
// The block's own hash is calculated based on its content.
func NewBlock(index int64, prevBlockHash string, transactions []*Transaction) (*Block, error) {
	if transactions == nil {
		// Allow blocks with no transactions (e.g. genesis block might not have app-level transactions)
		// but ensure it's an empty slice not a nil one for consistency.
		transactions = []*Transaction{}
	}

	block := &Block{
		Index:         index,
		Timestamp:     time.Now().UnixNano(),
		Transactions:  transactions,
		PrevBlockHash: prevBlockHash,
		// Hash will be calculated next
	}

	// Calculate the Merkle root of the transactions in the block.
	// If there are no transactions, use a hash of an empty string or a predefined empty root.
	var txHashes []string
	if len(transactions) > 0 {
		txHashes = GetTransactionHashes(transactions) // Assumes GetTransactionHashes is in crypto_utils.go or this package
	}
	merkleRoot := MerkleRoot(txHashes) // Assumes MerkleRoot is in crypto_utils.go or this package

	// Calculate the block's hash using its content.
	// The hash is based on Index, Timestamp, PrevBlockHash, and MerkleRoot of transactions.
	block.Hash = HashBlockContent(block.Index, block.Timestamp, block.PrevBlockHash, merkleRoot)

	return block, nil
}

// IsValid checks basic validity of the block structure and its hash.
// It does not validate individual transactions here, that's a separate concern.
func (b *Block) IsValid(prevBlock *Block) error {
	if b.Index != prevBlock.Index+1 {
		return fmt.Errorf("invalid block index: expected %d, got %d", prevBlock.Index+1, b.Index)
	}
	if b.PrevBlockHash != prevBlock.Hash {
		return fmt.Errorf("invalid previous block hash: expected %s, got %s", prevBlock.Hash, b.PrevBlockHash)
	}
	if b.Timestamp <= prevBlock.Timestamp && prevBlock.Index > 0 { // Allow genesis to have any timestamp
		return fmt.Errorf("invalid block timestamp: block %d timestamp %d before or same as prev block %d timestamp %d", b.Index, b.Timestamp, prevBlock.Index, prevBlock.Timestamp)
	}

	// Recalculate hash to verify integrity
	var txHashes []string
	if len(b.Transactions) > 0 {
		txHashes = GetTransactionHashes(b.Transactions)
	}
	merkleRoot := MerkleRoot(txHashes)
	expectedHash := HashBlockContent(b.Index, b.Timestamp, b.PrevBlockHash, merkleRoot)

	if b.Hash != expectedHash {
		return fmt.Errorf("invalid block hash: expected %s, got %s", expectedHash, b.Hash)
	}

	// Validate all transactions within the block
	for i, tx := range b.Transactions {
		if err := tx.IsValid(); err != nil {
			return fmt.Errorf("block contains invalid transaction at index %d: %w", i, err)
		}
		// Optionally, verify transaction signatures here as well if not done before adding to mempool
		// validSig, err := tx.VerifySignature()
		// if err != nil || !validSig {
		//  return fmt.Errorf("block contains transaction with invalid signature at index %d: %v", i, err)
		// }
	}

	return nil
}
