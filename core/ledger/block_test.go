package ledger

import (
	"testing"
	"time"
	// "fmt" // For debugging
)

func TestNewBlock(t *testing.T) {
	// Create some dummy transactions
	tx1, _ := NewTransaction("sender1", PostCreated, []byte("payload1"))
	tx2, _ := NewTransaction("sender2", CommentAdded, []byte("payload2"))
	transactions := []*Transaction{tx1, tx2}

	prevBlock := &Block{ // Sample previous block
		Index:     0,
		Timestamp: time.Now().UnixNano() - 1000, // Ensure previous block is older
		Hash:      "genesis_hash_placeholder",
	}

	block, err := NewBlock(prevBlock.Index+1, prevBlock.Hash, transactions)
	if err != nil {
		t.Fatalf("NewBlock() error = %v", err)
	}

	if block == nil {
		t.Fatalf("NewBlock() returned nil block")
	}
	if block.Index != prevBlock.Index+1 {
		t.Errorf("Block Index = %d, want %d", block.Index, prevBlock.Index+1)
	}
	if block.Timestamp <= prevBlock.Timestamp {
		t.Errorf("Block Timestamp %d is not greater than previous block timestamp %d", block.Timestamp, prevBlock.Timestamp)
	}
	if len(block.Transactions) != len(transactions) {
		t.Errorf("Block Transactions count = %d, want %d", len(block.Transactions), len(transactions))
	}
	if block.PrevBlockHash != prevBlock.Hash {
		t.Errorf("Block PrevBlockHash = %s, want %s", block.PrevBlockHash, prevBlock.Hash)
	}
	if block.Hash == "" {
		t.Errorf("Block Hash is empty")
	}

	// Verify hash calculation consistency
	txHashes := GetTransactionHashes(transactions)
	merkleRoot := MerkleRoot(txHashes)
	expectedHash := HashBlockContent(block.Index, block.Timestamp, block.PrevBlockHash, merkleRoot)
	if block.Hash != expectedHash {
		t.Errorf("Block Hash = %s, want %s (recalculated)", block.Hash, expectedHash)
	}

	// Test with no transactions
	blockNoTx, err := NewBlock(prevBlock.Index+1, prevBlock.Hash, []*Transaction{})
	if err != nil {
		t.Fatalf("NewBlock() with no transactions error = %v", err)
	}
	if len(blockNoTx.Transactions) != 0 {
		t.Errorf("Block with no transactions has non-empty Transactions field")
	}
	emptyMerkleRoot := MerkleRoot([]string{})
	expectedHashNoTx := HashBlockContent(blockNoTx.Index, blockNoTx.Timestamp, blockNoTx.PrevBlockHash, emptyMerkleRoot)
	if blockNoTx.Hash != expectedHashNoTx {
		t.Errorf("Block (no tx) Hash = %s, want %s", blockNoTx.Hash, expectedHashNoTx)
	}
}

func TestBlock_IsValid(t *testing.T) {
	// Setup: Create a valid previous block (like a genesis block)
	genesisTx, _ := NewTransaction("genesisSender", PostCreated, []byte("genesis payload"))
	_ = genesisTx.Sign([]byte("dummyKey"))
	prevBlock, _ := NewBlock(0, "0", []*Transaction{genesisTx})
	// Ensure prevBlock's timestamp is definitely in the past for subsequent blocks
	prevBlock.Timestamp = time.Now().UnixNano() - 10000
	prevBlock.Hash = HashBlockContent(prevBlock.Index, prevBlock.Timestamp, prevBlock.PrevBlockHash, MerkleRoot(GetTransactionHashes(prevBlock.Transactions)))


	// Create a valid current block based on prevBlock
	validTx1, _ := NewTransaction("senderA", PostCreated, []byte("valid payload 1"))
	_ = validTx1.Sign([]byte("dummyKey"))
	validTx2, _ := NewTransaction("senderB", CommentAdded, []byte("valid payload 2"))
	_ = validTx2.Sign([]byte("dummyKey"))

	validBlock, _ := NewBlock(prevBlock.Index+1, prevBlock.Hash, []*Transaction{validTx1, validTx2})
	// Ensure this block's timestamp is after prevBlock for test determinism
	validBlock.Timestamp = prevBlock.Timestamp + 500
	validBlock.Hash = HashBlockContent(validBlock.Index, validBlock.Timestamp, validBlock.PrevBlockHash, MerkleRoot(GetTransactionHashes(validBlock.Transactions)))


	tests := []struct {
		name      string
		blockFunc func() *Block // Function to generate the block for the test case
		prevBlock *Block
		wantErr   bool
	}{
		{
			name: "valid block",
			blockFunc: func() *Block {
				// Return a copy to avoid modification across tests if block was a pointer
				bCopy := *validBlock
				return &bCopy
			},
			prevBlock: prevBlock,
			wantErr:   false,
		},
		{
			name: "invalid index",
			blockFunc: func() *Block {
				b := *validBlock
				b.Index = prevBlock.Index // Wrong index
				// Hash needs to be consistent with content for other checks to pass before index check
				b.Hash = HashBlockContent(b.Index, b.Timestamp, b.PrevBlockHash, MerkleRoot(GetTransactionHashes(b.Transactions)))
				return &b
			},
			prevBlock: prevBlock,
			wantErr:   true,
		},
		{
			name: "invalid prevBlockHash",
			blockFunc: func() *Block {
				b := *validBlock
				b.PrevBlockHash = "wrong_previous_hash"
				b.Hash = HashBlockContent(b.Index, b.Timestamp, b.PrevBlockHash, MerkleRoot(GetTransactionHashes(b.Transactions)))
				return &b
			},
			prevBlock: prevBlock,
			wantErr:   true,
		},
		{
			name: "invalid timestamp (before prev)",
			blockFunc: func() *Block {
				b := *validBlock
				b.Timestamp = prevBlock.Timestamp - 100 // Timestamp before previous block
				b.Hash = HashBlockContent(b.Index, b.Timestamp, b.PrevBlockHash, MerkleRoot(GetTransactionHashes(b.Transactions)))
				return &b
			},
			prevBlock: prevBlock,
			wantErr:   true,
		},
		{
			name: "tampered hash (block content modified after hash calculation)",
			blockFunc: func() *Block {
				b := *validBlock
				b.Hash = "tampered_hash_value" // Hash doesn't match content
				return &b
			},
			prevBlock: prevBlock,
			wantErr:   true,
		},
		{
			name: "block with invalid transaction (empty ID)",
			blockFunc: func() *Block {
				invalidTx := *validTx1
				invalidTx.ID = "" // Tamper transaction
				b, _ := NewBlock(prevBlock.Index+1, prevBlock.Hash, []*Transaction{&invalidTx})
				b.Timestamp = prevBlock.Timestamp + 500
				b.Hash = HashBlockContent(b.Index, b.Timestamp, b.PrevBlockHash, MerkleRoot(GetTransactionHashes(b.Transactions)))
				return b
			},
			prevBlock: prevBlock,
			wantErr:   true,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			blockToTest := tt.blockFunc()
			// fmt.Printf("Testing %s: Block Hash %s, Prev Hash %s, Index %d, Timestamp %d\n", tt.name, blockToTest.Hash, blockToTest.PrevBlockHash, blockToTest.Index, blockToTest.Timestamp)
			// fmt.Printf("Prev Block For Test: Hash %s, Index %d, Timestamp %d\n", tt.prevBlock.Hash, tt.prevBlock.Index, tt.prevBlock.Timestamp)

			err := blockToTest.IsValid(tt.prevBlock)
			if (err != nil) != tt.wantErr {
				t.Errorf("Block.IsValid() error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}
