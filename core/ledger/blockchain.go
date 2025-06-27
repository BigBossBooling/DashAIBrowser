package ledger

import (
	"fmt"
	"sync"
	"time"
)

// Blockchain represents the append-only chain of blocks.
type Blockchain struct {
	mu     sync.Mutex // For thread-safe access to the chain
	Blocks []*Block
	// TODO: Could add a map for quick block lookup by hash:
	// blockIndex map[string]*Block
}

// NewBlockchain creates and returns a new Blockchain, initialized with a genesis block.
func NewBlockchain() (*Blockchain, error) {
	genesisTransactions := []*Transaction{
		// Optionally, create a "coinbase-like" transaction for the genesis block
		// For now, keeping it simple with no specific genesis transactions.
	}
	genesisBlock, err := NewBlock(0, "0", genesisTransactions) // Index 0, PrevHash "0"
	if err != nil {
		return nil, fmt.Errorf("failed to create genesis block: %w", err)
	}
	// Manually set a known timestamp for genesis if desired for consistency in tests
	genesisBlock.Timestamp = time.Date(2024, 1, 1, 0, 0, 0, 0, time.UTC).UnixNano()
	// Recalculate hash if timestamp is manually set
	var txHashes []string
	if len(genesisBlock.Transactions) > 0 {
		txHashes = GetTransactionHashes(genesisBlock.Transactions)
	}
	merkleRoot := MerkleRoot(txHashes)
	genesisBlock.Hash = HashBlockContent(genesisBlock.Index, genesisBlock.Timestamp, genesisBlock.PrevBlockHash, merkleRoot)


	return &Blockchain{
		Blocks: []*Block{genesisBlock},
	}, nil
}

// GetLatestBlock returns the most recent block in the chain.
func (bc *Blockchain) GetLatestBlock() *Block {
	bc.mu.Lock()
	defer bc.mu.Unlock()
	if len(bc.Blocks) == 0 {
		return nil // Should not happen if genesis block is always present
	}
	return bc.Blocks[len(bc.Blocks)-1]
}

// AddBlock creates a new block with the given transactions and adds it to the blockchain.
// It performs validation before adding.
func (bc *Blockchain) AddBlock(transactions []*Transaction) (*Block, error) {
	bc.mu.Lock()
	defer bc.mu.Unlock()

	if len(bc.Blocks) == 0 {
		return nil, fmt.Errorf("blockchain is not initialized with a genesis block")
	}
	latestBlock := bc.Blocks[len(bc.Blocks)-1]

	// Validate transactions before adding them to a block
	for i, tx := range transactions {
		if err := tx.IsValid(); err != nil {
			return nil, fmt.Errorf("invalid transaction at index %d for new block: %w", i, err)
		}
		// In a real system, also verify signatures here if not done before (e.g. in a mempool)
		validSig, err := tx.VerifySignature()
		if err != nil {
			return nil, fmt.Errorf("error verifying signature for transaction %s: %w", tx.ID, err)
		}
		if !validSig {
			return nil, fmt.Errorf("invalid signature for transaction %s", tx.ID)
		}
	}

	newBlock, err := NewBlock(latestBlock.Index+1, latestBlock.Hash, transactions)
	if err != nil {
		return nil, fmt.Errorf("failed to create new block: %w", err)
	}

	// Validate the new block against the current latest block
	// The IsValid method on Block already checks index, prevhash, and its own hash.
	// Here, we pass the current latestBlock as the context for prevBlock.
	if err := newBlock.IsValid(latestBlock); err != nil {
		return nil, fmt.Errorf("newly created block is invalid: %w", err)
	}

	bc.Blocks = append(bc.Blocks, newBlock)
	fmt.Printf("Block #%d added to the blockchain.\nHash: %s\n", newBlock.Index, newBlock.Hash)
	return newBlock, nil
}

// IsChainValid checks the integrity of the entire blockchain.
// It verifies each block against its predecessor and validates hashes.
func (bc *Blockchain) IsChainValid() (bool, error) {
	bc.mu.Lock()
	defer bc.mu.Unlock()

	if len(bc.Blocks) == 0 {
		return false, fmt.Errorf("blockchain is empty, cannot validate")
	}

	// Check genesis block (basic check, could be more thorough if genesis has fixed properties)
	genesis := bc.Blocks[0]
	if genesis.Index != 0 || genesis.PrevBlockHash != "0" {
		return false, fmt.Errorf("genesis block invalid: index %d, prevHash %s", genesis.Index, genesis.PrevBlockHash)
	}
	// Recalculate genesis hash to verify integrity
	var txHashes []string
	if len(genesis.Transactions) > 0 {
		txHashes = GetTransactionHashes(genesis.Transactions)
	}
	merkleRoot := MerkleRoot(txHashes)
	expectedGenesisHash := HashBlockContent(genesis.Index, genesis.Timestamp, genesis.PrevBlockHash, merkleRoot)
	if genesis.Hash != expectedGenesisHash {
		return false, fmt.Errorf("genesis block hash mismatch: expected %s, got %s", expectedGenesisHash, genesis.Hash)
	}


	// Check subsequent blocks
	for i := 1; i < len(bc.Blocks); i++ {
		currentBlock := bc.Blocks[i]
		previousBlock := bc.Blocks[i-1]

		if err := currentBlock.IsValid(previousBlock); err != nil {
			return false, fmt.Errorf("chain validation failed at block %d: %w", currentBlock.Index, err)
		}
	}
	return true, nil
}

// GetBlockByIndex returns a block by its index. Returns nil if not found.
func (bc *Blockchain) GetBlockByIndex(index int64) *Block {
    bc.mu.Lock()
    defer bc.mu.Unlock()
    if index < 0 || index >= int64(len(bc.Blocks)) {
        return nil
    }
    return bc.Blocks[index]
}

// GetBlockByHash returns a block by its hash. Returns nil if not found.
// This would be more efficient with a blockIndex map.
func (bc *Blockchain) GetBlockByHash(hash string) *Block {
    bc.mu.Lock()
    defer bc.mu.Unlock()
    for _, block := range bc.Blocks {
        if block.Hash == hash {
            return block
        }
    }
    return nil
}

// GetTransactionByID searches the entire blockchain for a transaction by its ID.
// This is inefficient and primarily for debugging or specific lookup needs.
func (bc *Blockchain) GetTransactionByID(txID string) (*Transaction, *Block) {
    bc.mu.Lock()
    defer bc.mu.Unlock()
    for _, block := range bc.Blocks {
        for _, tx := range block.Transactions {
            if tx.ID == txID {
                return tx, block
            }
        }
    }
    return nil, nil
}
