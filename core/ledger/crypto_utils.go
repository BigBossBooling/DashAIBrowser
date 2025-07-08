package ledger

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"sort"
	"strings"
)

// CalculateSHA256Hash calculates a SHA256 hash for the given byte slice
// and returns it as a hex-encoded string.
func CalculateSHA256Hash(data []byte) string {
	hashBytes := sha256.Sum256(data)
	return hex.EncodeToString(hashBytes[:])
}

// prepareDataForHashing serializes an interface into a canonical JSON string for hashing.
// It ensures that map keys are sorted to produce a deterministic output.
// Using JSON for simplicity; a more performant binary serialization might be used in production.
func prepareDataForHashing(data interface{}) ([]byte, error) {
	// Using json.Marshal for now. For true canonical representation,
	// especially with maps, fields should be sorted.
	// A more robust approach would be to define a specific serialization format
	// or use a library that guarantees canonical serialization.
	// For simple structs without maps, direct json.Marshal is often sufficient if field order is fixed.

	// To handle map key sorting for deterministic hashing if maps were directly in the struct:
	// This is a simplified example. A full solution would recursively handle nested maps.
	// if m, ok := data.(map[string]interface{}); ok {
	//     return json.Marshal(sortMapKeys(m))
	// }
	return json.Marshal(data)
}

// HashTransactionContent creates a hash for the transaction's content,
// typically used to generate the transaction ID before signing.
// It uses the deterministic string representation.
func HashTransactionContent(timestamp int64, senderPublicKey string, txType TransactionType, payload []byte) string {
	deterministicInput := GenerateDeterministicTransactionIDInput(timestamp, senderPublicKey, txType, payload)
	return CalculateSHA256Hash([]byte(deterministicInput))
}

// HashBlockContent creates a hash for the block's content.
// It uses the deterministic string representation of the block header.
func HashBlockContent(index int64, timestamp int64, prevBlockHash string, transactionMerkleRoot string) string {
	deterministicInput := GenerateDeterministicBlockHeaderInput(index, timestamp, prevBlockHash, transactionMerkleRoot)
	return CalculateSHA256Hash([]byte(deterministicInput))
}

// MerkleRoot calculates a simple Merkle root for a list of transaction hashes.
// This is a basic implementation; more optimized versions exist.
// If there are no transactions, it returns an empty string or a hash of an empty string.
func MerkleRoot(transactionHashes []string) string {
	if len(transactionHashes) == 0 {
		return CalculateSHA256Hash([]byte{}) // Or an empty string, depending on convention
	}
	if len(transactionHashes) == 1 {
		return transactionHashes[0]
	}

	var nextLevel []string
	for i := 0; i < len(transactionHashes); i += 2 {
		if i+1 < len(transactionHashes) {
			combinedHash := CalculateSHA256Hash([]byte(transactionHashes[i] + transactionHashes[i+1]))
			nextLevel = append(nextLevel, combinedHash)
		} else {
			// If odd number of hashes, duplicate the last one or hash it with itself
			// Duplicating is simpler for this example.
			nextLevel = append(nextLevel, transactionHashes[i])
		}
	}
	return MerkleRoot(nextLevel)
}

// Helper to get transaction hashes for block hashing
func GetTransactionHashes(transactions []*Transaction) []string {
	hashes := make([]string, len(transactions))
	for i, tx := range transactions {
		hashes[i] = tx.ID // Assuming tx.ID is already the hash of the transaction content
	}
	// It's crucial that these IDs are themselves deterministic and consistently ordered
	// if they are to be part of the block hash directly without a Merkle tree.
	// If using MerkleRoot, the order passed to MerkleRoot matters initially,
	// but the root itself is deterministic.
	// For BlockHeaderForHashing, we sort these hashes before hashing the block header.
	return hashes
}


// GenerateDeterministicTransactionIDInput creates a canonical string representation of transaction data for ID generation.
// This ensures that the same transaction data always produces the same hash ID.
// It explicitly concatenates relevant fields in a fixed order.
func GenerateDeterministicTransactionIDInput(timestamp int64, senderPublicKey string, txType TransactionType, payload []byte) string {
    var sb strings.Builder
    sb.WriteString(fmt.Sprintf("%d", timestamp))
    sb.WriteString("|")
    sb.WriteString(senderPublicKey)
    sb.WriteString("|")
    sb.WriteString(string(txType))
    sb.WriteString("|")
    sb.WriteString(hex.EncodeToString(payload)) // Ensure payload is consistently represented
    return sb.String()
}

// GenerateDeterministicBlockHeaderInput creates a canonical string representation of block header data for hashing.
func GenerateDeterministicBlockHeaderInput(index int64, timestamp int64, prevBlockHash string, transactionMerkleRoot string) string {
	var sb strings.Builder
	sb.WriteString(fmt.Sprintf("%d", index))
	sb.WriteString("|")
	sb.WriteString(fmt.Sprintf("%d", timestamp))
	sb.WriteString("|")
	sb.WriteString(prevBlockHash)
	sb.WriteString("|")
	sb.WriteString(transactionMerkleRoot)
	// sb.WriteString(fmt.Sprintf("%d", nonce)) // If nonce is used
	return sb.String()
}
