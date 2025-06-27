package ledger

import (
	"crypto/sha256"
	"encoding/hex"
	"reflect"
	"testing"
)

func TestCalculateSHA256Hash(t *testing.T) {
	data := []byte("hello world")
	expectedHashBytes := sha256.Sum256(data)
	expectedHash := hex.EncodeToString(expectedHashBytes[:])

	hash := CalculateSHA256Hash(data)
	if hash != expectedHash {
		t.Errorf("CalculateSHA256Hash() = %v, want %v", hash, expectedHash)
	}

	emptyDataHash := CalculateSHA256Hash([]byte{})
	if len(emptyDataHash) != 64 { // SHA256 hex string length
		t.Errorf("CalculateSHA256Hash for empty data produced hash of wrong length: %s", emptyDataHash)
	}
}

func TestDeterministicTransactionIDInput(t *testing.T) {
	ts := int64(1678886400000000000) // Example timestamp
	sender := "testSenderPublicKey"
	txType := PostCreated
	payload := []byte("test payload")

	input1 := GenerateDeterministicTransactionIDInput(ts, sender, txType, payload)
	input2 := GenerateDeterministicTransactionIDInput(ts, sender, txType, payload)

	if input1 != input2 {
		t.Errorf("GenerateDeterministicTransactionIDInput is not deterministic for same inputs:\n1: %s\n2: %s", input1, input2)
	}

	input3 := GenerateDeterministicTransactionIDInput(ts+1, sender, txType, payload)
	if input1 == input3 {
		t.Errorf("GenerateDeterministicTransactionIDInput produced same output for different timestamps")
	}
}

func TestHashTransactionContent(t *testing.T) {
	ts := int64(1678886400000000000)
	sender := "testSender"
	txType := PostCreated
	payload := []byte("hello")

	hash1 := HashTransactionContent(ts, sender, txType, payload)
	hash2 := HashTransactionContent(ts, sender, txType, payload)

	if hash1 == "" {
		t.Errorf("HashTransactionContent returned empty string")
	}
	if hash1 != hash2 {
		t.Errorf("HashTransactionContent not deterministic: %s != %s", hash1, hash2)
	}

	// Check that changing any part changes the hash
	hashPayloadChanged := HashTransactionContent(ts, sender, txType, []byte("world"))
	if hash1 == hashPayloadChanged {
		t.Errorf("HashTransactionContent did not change with payload")
	}
	hashTimestampChanged := HashTransactionContent(ts+1, sender, txType, payload)
	if hash1 == hashTimestampChanged {
		t.Errorf("HashTransactionContent did not change with timestamp")
	}
}


func TestDeterministicBlockHeaderInput(t *testing.T) {
	input1 := GenerateDeterministicBlockHeaderInput(1, 12345, "prevHash", "merkleRoot")
	input2 := GenerateDeterministicBlockHeaderInput(1, 12345, "prevHash", "merkleRoot")
	if input1 != input2 {
		t.Errorf("GenerateDeterministicBlockHeaderInput is not deterministic for same inputs")
	}
	input3 := GenerateDeterministicBlockHeaderInput(2, 12345, "prevHash", "merkleRoot")
	if input1 == input3 {
		t.Errorf("GenerateDeterministicBlockHeaderInput produced same output for different index")
	}
}

func TestHashBlockContent(t *testing.T) {
	hash1 := HashBlockContent(0, 1678886400, "genesis_prev_hash", "merkle_root_1")
	hash2 := HashBlockContent(0, 1678886400, "genesis_prev_hash", "merkle_root_1")

	if hash1 == "" {
		t.Errorf("HashBlockContent returned an empty hash")
	}
	if hash1 != hash2 {
		t.Errorf("HashBlockContent is not deterministic: %s != %s", hash1, hash2)
	}

	hashMerkleChanged := HashBlockContent(0, 1678886400, "genesis_prev_hash", "merkle_root_2")
	if hash1 == hashMerkleChanged {
		t.Errorf("HashBlockContent did not change with Merkle root")
	}
}


func TestMerkleRoot(t *testing.T) {
	tests := []struct {
		name     string
		txHashes []string
		want     string
	}{
		{
			name:     "empty transactions",
			txHashes: []string{},
			want:     CalculateSHA256Hash([]byte{}),
		},
		{
			name:     "single transaction",
			txHashes: []string{"hash1"},
			want:     "hash1",
		},
		{
			name:     "two transactions",
			txHashes: []string{"hash1", "hash2"},
			want:     CalculateSHA256Hash([]byte("hash1" + "hash2")),
		},
		{
			name:     "three transactions", // h1, h2, h3 -> h(h1,h2), h3 -> h(h(h1,h2),h3) -- no, h3 is paired with itself or last available
			// Corrected logic: h1, h2, h3 -> h(h1,h2), h3 -> h(h(h1,h2), h3) is not standard.
			// Standard is often: h1, h2, h3, h3 (if odd, duplicate last)
			// Then: h(h1,h2), h(h3,h3)
			// Then: h(h(h1,h2), h(h3,h3))
			// My MerkleRoot implementation is simpler: it takes the odd one to the next level.
			// h1,h2,h3 -> h(h1,h2), h3 -> h(h(h1,h2), h3)
			txHashes: []string{"a", "b", "c"},
			// Level 1: h(a,b), c
			// Level 2: h(h(a,b), c)
			want: CalculateSHA256Hash([]byte(CalculateSHA256Hash([]byte("a"+"b")) + "c")),
		},
		{
			name:     "four transactions",
			txHashes: []string{"a", "b", "c", "d"},
			// Level 1: h(a,b), h(c,d)
			// Level 2: h(h(a,b), h(c,d))
			want: CalculateSHA256Hash([]byte(CalculateSHA256Hash([]byte("a"+"b")) + CalculateSHA256Hash([]byte("c"+"d")))),
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := MerkleRoot(tt.txHashes); got != tt.want {
				t.Errorf("MerkleRoot() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestGetTransactionHashes(t *testing.T) {
	tx1, _ := NewTransaction("sender1", PostCreated, []byte("payload1"))
	tx2, _ := NewTransaction("sender2", CommentAdded, []byte("payload2"))
	transactions := []*Transaction{tx1, tx2}

	hashes := GetTransactionHashes(transactions)
	if len(hashes) != 2 {
		t.Fatalf("Expected 2 hashes, got %d", len(hashes))
	}
	if hashes[0] != tx1.ID {
		t.Errorf("Expected hash[0] to be tx1.ID, got %s", hashes[0])
	}
	if hashes[1] != tx2.ID {
		t.Errorf("Expected hash[1] to be tx2.ID, got %s", hashes[1])
	}
}


func TestSignAndVerifyData_Placeholder(t *testing.T) {
	dataToSign := []byte("test data for signing")
	var dummyPrivateKey []byte // Unused by placeholder

	signature, err := SignData(dataToSign, dummyPrivateKey)
	if err != nil {
		t.Fatalf("SignData failed: %v", err)
	}
	if len(signature) == 0 {
		t.Errorf("SignData returned empty signature")
	}

	// Test successful verification
	valid, err := VerifySignature("dummyPubKey", dataToSign, signature)
	if err != nil {
		t.Fatalf("VerifySignature failed: %v", err)
	}
	if !valid {
		t.Errorf("Expected signature to be valid, but it was not")
	}

	// Test verification failure with wrong data
	tamperedData := []byte("tampered test data")
	invalid, err := VerifySignature("dummyPubKey", tamperedData, signature)
	if err != nil {
		// This might actually pass if the placeholder doesn't use the data in sig comparison
		// Our current placeholder does use it.
		t.Logf("VerifySignature with tampered data error (expected for strict placeholder): %v", err)
	}
	if invalid {
		t.Errorf("Expected signature to be invalid for tampered data, but it was valid")
	}

	// Test verification failure with wrong signature
	tamperedSignature := []byte("completely_wrong_signature")
	invalid, err = VerifySignature("dummyPubKey", dataToSign, tamperedSignature)
	if err != nil {
		t.Logf("VerifySignature with tampered signature error (expected for strict placeholder): %v", err)
	}
	if invalid {
		t.Errorf("Expected tampered signature to be invalid, but it was valid")
	}
}


// TestPrepareDataForHashing_Determinism (Optional, as current impl uses json.Marshal which is tricky for maps)
// This test is more relevant if we had a custom canonical serialization.
// For now, we rely on struct field order and no maps in the hashed structs.
func TestPrepareDataForHashing_Determinism(t *testing.T) {
	// Test with a struct that has a defined order (no maps)
	data1 := struct {
		A int
		B string
	}{10, "hello"}

	data2 := struct {
		A int
		B string
	}{10, "hello"}

	bytes1, err1 := prepareDataForHashing(data1)
	bytes2, err2 := prepareDataForHashing(data2)

	if err1 != nil || err2 != nil {
		t.Fatalf("prepareDataForHashing failed: %v, %v", err1, err2)
	}

	if !reflect.DeepEqual(bytes1, bytes2) {
		t.Errorf("prepareDataForHashing is not deterministic for simple structs:\nBytes1: %s\nBytes2: %s", string(bytes1), string(bytes2))
	}

	// Example of why map makes json.Marshal non-deterministic without custom handling
	mapData1 := map[string]int{"a": 1, "b": 2}
	mapData2 := map[string]int{"b": 2, "a": 1} // Same content, different order potential

	// Note: json.Marshal *might* sort map keys for top-level maps, but this is not guaranteed
	// across all Go versions or for nested maps.
	// For truly canonical JSON, a custom marshaler or specific library is needed.
	// Our current use in HashBlockContent and HashTransactionContent avoids direct marshaling of maps
	// by using deterministic string concatenation helpers.

	// This test might pass or fail depending on Go's json.Marshal behavior for maps.
	// It's here to illustrate the point.
	mapBytes1, _ := json.Marshal(mapData1)
	mapBytes2, _ := json.Marshal(mapData2)
	if !reflect.DeepEqual(mapBytes1, mapBytes2) {
		t.Logf("json.Marshal for maps is not deterministic by default (which is expected): \nMapBytes1: %s\nMapBytes2: %s", string(mapBytes1), string(mapBytes2))
	} else {
		t.Logf("json.Marshal for these simple maps was deterministic (might not always be true for complex cases): \nMapBytes1: %s\nMapBytes2: %s", string(mapBytes1), string(mapBytes2))
	}
}
