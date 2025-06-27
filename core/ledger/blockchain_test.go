package ledger

import (
	"bytes"
	"testing"
	"time"
)

func TestNewBlockchain(t *testing.T) {
	bc, err := NewBlockchain()
	if err != nil {
		t.Fatalf("NewBlockchain() error = %v", err)
	}
	if bc == nil {
		t.Fatal("NewBlockchain() returned nil")
	}
	if len(bc.Blocks) != 1 {
		t.Fatalf("Blockchain should have 1 block (genesis) upon creation, got %d", len(bc.Blocks))
	}

	genesisBlock := bc.Blocks[0]
	if genesisBlock.Index != 0 {
		t.Errorf("Genesis block Index = %d, want 0", genesisBlock.Index)
	}
	if genesisBlock.PrevBlockHash != "0" {
		t.Errorf("Genesis block PrevBlockHash = %s, want \"0\"", genesisBlock.PrevBlockHash)
	}
	if genesisBlock.Hash == "" {
		t.Errorf("Genesis block Hash is empty")
	}
	// Check if genesis hash is valid for its content
	expectedGenesisHash := HashBlockContent(genesisBlock.Index, genesisBlock.Timestamp, genesisBlock.PrevBlockHash, MerkleRoot(GetTransactionHashes(genesisBlock.Transactions)))
	if genesisBlock.Hash != expectedGenesisHash {
		t.Errorf("Genesis block hash mismatch. Got %s, expected %s", genesisBlock.Hash, expectedGenesisHash)
	}
}

func TestBlockchain_AddBlock(t *testing.T) {
	bc, _ := NewBlockchain()

	// Create some valid transactions
	tx1Payload := []byte("first transaction data")
	tx1, _ := NewTransaction("sender1", PostCreated, tx1Payload)
	_ = tx1.Sign([]byte("dummyPrivateKey")) // Sign with placeholder

	tx2Payload := []byte("second transaction data")
	tx2, _ := NewTransaction("sender2", CommentAdded, tx2Payload)
	_ = tx2.Sign([]byte("dummyPrivateKey"))

	transactions := []*Transaction{tx1, tx2}

	newBlock, err := bc.AddBlock(transactions)
	if err != nil {
		t.Fatalf("AddBlock() error = %v", err)
	}
	if newBlock == nil {
		t.Fatal("AddBlock() returned nil block")
	}

	if len(bc.Blocks) != 2 {
		t.Errorf("Blockchain should have 2 blocks after adding one, got %d", len(bc.Blocks))
	}
	latestBlock := bc.GetLatestBlock()
	if latestBlock.Index != 1 {
		t.Errorf("New block Index = %d, want 1", latestBlock.Index)
	}
	if latestBlock.PrevBlockHash != bc.Blocks[0].Hash {
		t.Errorf("New block PrevBlockHash = %s, want %s", latestBlock.PrevBlockHash, bc.Blocks[0].Hash)
	}
	if len(latestBlock.Transactions) != 2 {
		t.Errorf("New block should have 2 transactions, got %d", len(latestBlock.Transactions))
	}
	if latestBlock.Hash == "" {
		t.Errorf("New block Hash is empty")
	}

	// Test adding a block with an invalid transaction (e.g., unsigned or bad structure)
	invalidTx, _ := NewTransaction("sender3", PostCreated, []byte("invalid payload"))
	// Not signing invalidTx, or tampering it:
	// invalidTx.Signature = []byte{} // or tx.ID = "" to fail IsValid()

	_, err = bc.AddBlock([]*Transaction{invalidTx})
	if err == nil {
		t.Errorf("Expected error when adding block with invalid (unsigned) transaction, but got nil")
	}

	// Test adding a block that would be invalid (e.g., wrong prev hash - harder to test directly here
	// as AddBlock creates the new block itself. IsValid on the chain tests this better.)
}

func TestBlockchain_IsChainValid(t *testing.T) {
	bc, _ := NewBlockchain()

	// Add a few valid blocks
	for i := 0; i < 3; i++ {
		payload := []byte("Block " + string(rune(i+1)) + " transaction")
		tx, _ := NewTransaction("sender"+string(rune(i)), PostCreated, payload)
		_ = tx.Sign([]byte("dummyKey"))
		_, err := bc.AddBlock([]*Transaction{tx})
		if err != nil {
			t.Fatalf("Failed to add valid block during test setup: %v", err)
		}
		// Add a small delay to ensure timestamps are different for blocks if test runs very fast
		time.Sleep(1 * time.Millisecond)
	}

	valid, err := bc.IsChainValid()
	if err != nil {
		t.Fatalf("IsChainValid() on a valid chain returned error: %v", err)
	}
	if !valid {
		t.Errorf("IsChainValid() on a valid chain returned false")
	}

	// --- Test case: Tamper with a block's hash ---
	if len(bc.Blocks) > 1 {
		originalHash := bc.Blocks[1].Hash
		bc.Blocks[1].Hash = "tampered_hash_123"
		valid, err = bc.IsChainValid()
		if err == nil {
			t.Errorf("Expected error for tampered block hash, but got nil")
		}
		if valid {
			t.Errorf("Expected chain to be invalid with tampered block hash, but was valid")
		}
		bc.Blocks[1].Hash = originalHash // Restore
	}

	// --- Test case: Tamper with a block's PrevBlockHash ---
	if len(bc.Blocks) > 1 {
		originalPrevHash := bc.Blocks[1].PrevBlockHash
		bc.Blocks[1].PrevBlockHash = "tampered_prev_hash_456"
		valid, err = bc.IsChainValid()
		if err == nil {
			t.Errorf("Expected error for tampered PrevBlockHash, but got nil")
		}
		if valid {
			t.Errorf("Expected chain to be invalid with tampered PrevBlockHash, but was valid")
		}
		bc.Blocks[1].PrevBlockHash = originalPrevHash // Restore
	}

	// --- Test case: Tamper with a transaction within a block (after block was added) ---
	if len(bc.Blocks) > 1 && len(bc.Blocks[1].Transactions) > 0 {
		originalTxID := bc.Blocks[1].Transactions[0].ID
		// This kind of tampering is subtle. If Block.IsValid recalculates Merkle root and block hash,
		// and IsChainValid calls Block.IsValid, this should be caught.
		// Our current Block.IsValid re-calculates the hash based on its *current* transactions.
		// So, if a transaction *inside* a block changes, and the block's hash is not updated,
		// Block.IsValid (when called by IsChainValid) should detect the hash mismatch.

		// Let's try to make the block's hash invalid by changing a tx that was used to compute it.
		// This requires the block's hash to be recalculated to show the test logic.
		// A more direct test: change block.Hash after changing a tx.

		// Get the block, tamper a tx, then check if the original block hash is still valid for it.
		blockToTamper := *bc.Blocks[1] // Make a copy
		originalTxPayload := blockToTamper.Transactions[0].Payload
		blockToTamper.Transactions[0].Payload = []byte("tampered payload in existing block")
		// The block's stored hash (blockToTamper.Hash) is now stale.
		// IsValid on blockToTamper should fail because its stored hash doesn't match its new content.
		err = blockToTamper.IsValid(bc.Blocks[0])
		if err == nil {
			t.Errorf("Expected error when validating a block with tampered transaction content (hash mismatch), got nil")
		}
		// Restore for next tests if needed, though we used a copy.
		bc.Blocks[1].Transactions[0].Payload = originalTxPayload
	}


	// --- Test case: Invalid timestamp sequence ---
	if len(bc.Blocks) > 2 {
		originalTimestamp := bc.Blocks[2].Timestamp
		bc.Blocks[2].Timestamp = bc.Blocks[1].Timestamp - 100 // Make it older than previous
		// The hash will be wrong now, so IsValid should catch that first.
		// To specifically test timestamp, the hash would need to be "correct" for the wrong timestamp.
		// This is tricky without re-calculating hash. The current IsValid checks hash first.
		// For a more direct timestamp test, we'd need to construct a block with bad timestamp but correct hash for that state.
		// The current Block.IsValid does check timestamp against prevBlock.Timestamp.
		valid, err = bc.IsChainValid()
		if err == nil {
			t.Errorf("Expected error for invalid timestamp sequence, got nil")
		}
		if valid {
			t.Errorf("Expected chain to be invalid with out-of-order timestamp")
		}
		bc.Blocks[2].Timestamp = originalTimestamp // Restore
		// Must re-validate the chain after restore if we continue tests that depend on its validity
		// For now, this is the last tampering test for IsChainValid.
	}
}


func TestBlockchain_Getters(t *testing.T) {
	bc, _ := NewBlockchain()
	tx1, _ := NewTransaction("s1", PostCreated, []byte("p1"))
	_ = tx1.Sign([]byte("key"))
	b1, _ := bc.AddBlock([]*Transaction{tx1})

	tx2, _ := NewTransaction("s2", CommentAdded, []byte("p2"))
	_ = tx2.Sign([]byte("key"))
	b2, _ := bc.AddBlock([]*Transaction{tx2})

	// Test GetLatestBlock
	if bc.GetLatestBlock().Hash != b2.Hash {
		t.Errorf("GetLatestBlock returned wrong block")
	}

	// Test GetBlockByIndex
	if bc.GetBlockByIndex(0).Hash != bc.Blocks[0].Hash { // Genesis
		t.Errorf("GetBlockByIndex(0) failed")
	}
	if bc.GetBlockByIndex(1).Hash != b1.Hash {
		t.Errorf("GetBlockByIndex(1) failed")
	}
	if bc.GetBlockByIndex(2).Hash != b2.Hash {
		t.Errorf("GetBlockByIndex(2) failed")
	}
	if bc.GetBlockByIndex(3) != nil {
		t.Errorf("GetBlockByIndex(3) should be nil for out of bounds")
	}
	if bc.GetBlockByIndex(-1) != nil {
		t.Errorf("GetBlockByIndex(-1) should be nil for out of bounds")
	}

	// Test GetBlockByHash
	if bc.GetBlockByHash(b1.Hash).Index != b1.Index {
		t.Errorf("GetBlockByHash for b1 failed")
	}
	if bc.GetBlockByHash("nonexistenthash") != nil {
		t.Errorf("GetBlockByHash for nonexistent hash should be nil")
	}

	// Test GetTransactionByID
	foundTx, inBlock := bc.GetTransactionByID(tx1.ID)
	if foundTx == nil || foundTx.ID != tx1.ID {
		t.Errorf("GetTransactionByID failed to find tx1 or found wrong tx")
	}
	if inBlock == nil || inBlock.Hash != b1.Hash {
		t.Errorf("GetTransactionByID returned wrong block for tx1")
	}
	nonExistentTx, _ := bc.GetTransactionByID("nonexistentTXID")
	if nonExistentTx != nil {
		t.Errorf("GetTransactionByID found a non-existent transaction")
	}
}

// Helper to ensure transactions are actually different for some tests
func createUniqueTransaction(index int) *Transaction {
	tx, _ := NewTransaction(fmt.Sprintf("sender%d", index), PostCreated, []byte(fmt.Sprintf("payload%d", index)))
	_ = tx.Sign([]byte("dummy"))
	return tx
}

func TestBlockchain_AddBlock_TransactionValidation(t *testing.T) {
	bc, _ := NewBlockchain()

	// Case 1: Transaction with empty ID (should fail IsValid())
	txEmptyID := createUniqueTransaction(1)
	txEmptyID.ID = ""
	// Note: tx.Sign() would fail if ID is empty, so we are testing IsValid() check within AddBlock
	// For this specific test to work as intended (invalid tx failing AddBlock),
	// tx.IsValid() must be robust.

	// To properly test AddBlock's call to tx.IsValid(), we need a tx that *would* be invalid.
	// Our current tx.IsValid() checks ID, Timestamp, SenderPublicKey, Type.
	// NewTransaction sets these correctly. Let's make one invalid *after* creation.

	txBadTimestamp := createUniqueTransaction(2)
	txBadTimestamp.Timestamp = 0 // Invalid timestamp

	txNoSender := createUniqueTransaction(3)
	txNoSender.SenderPublicKey = ""
	// Recalculate ID for txNoSender to be consistent with its current (bad) state for IsValid() to focus on sender.
	// This is a bit artificial as NewTransaction would prevent this.
	// txNoSender.ID = HashTransactionContent(txNoSender.Timestamp, txNoSender.SenderPublicKey, txNoSender.Type, txNoSender.Payload)
	// Actually, tx.IsValid() doesn't re-check ID consistency, it just checks if ID is non-empty.

	tests := []struct {
		name    string
		txs     []*Transaction
		wantErr bool
	}{
		{
			name:    "add block with valid transaction",
			txs:     []*Transaction{createUniqueTransaction(0)},
			wantErr: false,
		},
		{
			name:    "add block with transaction with empty ID (forced)",
			txs:     []*Transaction{txEmptyID}, // This tx's IsValid() should fail
			wantErr: true,
		},
		{
			name:    "add block with transaction with bad timestamp",
			txs:     []*Transaction{txBadTimestamp},
			wantErr: true,
		},
		// The following test might pass if tx.Sign() is not called, as VerifySignature would fail.
		// If signature is nil, VerifySignature fails.
		{
			name:    "add block with unsigned transaction",
			txs:     []*Transaction{NewTransactionUnsigned("sender", PostCreated, []byte("unsigned"))},
			wantErr: true,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Get current chain length to ensure we try to add to a fresh state relative to genesis
			// This is not perfect if tests run in parallel and modify the same `bc` instance.
			// For robust parallel tests, each would need its own `bc`.
			// For now, assuming sequential execution of t.Run blocks.

			_, err := bc.AddBlock(tt.txs)
			if (err != nil) != tt.wantErr {
				t.Errorf("Blockchain.AddBlock() error = %v, wantErr %v", err, tt.wantErr)
			}
			// If successful, subsequent tests might operate on a longer chain. This is okay for these specific cases.
		})
	}
}

// Helper for TestBlockchain_AddBlock_TransactionValidation
func NewTransactionUnsigned(senderPublicKey string, txType TransactionType, payload []byte) *Transaction {
    ts := time.Now().UnixNano()
    tx := &Transaction{
        Timestamp:       ts,
        SenderPublicKey: senderPublicKey,
        Type:            txType,
        Payload:         payload,
    }
    tx.ID = HashTransactionContent(tx.Timestamp, tx.SenderPublicKey, tx.Type, tx.Payload)
    // tx.Signature remains nil
    return tx
}
