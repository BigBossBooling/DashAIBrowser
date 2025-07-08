package ledger

import (
	"bytes"
	"testing"
	"time"
)

func TestNewTransaction(t *testing.T) {
	senderPK := "testSenderPK"
	txType := PostCreated
	payload := []byte("This is a test post.")

	tx, err := NewTransaction(senderPK, txType, payload)
	if err != nil {
		t.Fatalf("NewTransaction() error = %v", err)
	}

	if tx == nil {
		t.Fatalf("NewTransaction() returned nil transaction")
	}
	if tx.ID == "" {
		t.Errorf("Transaction ID is empty")
	}
	if tx.Timestamp <= 0 {
		t.Errorf("Transaction timestamp is invalid: %d", tx.Timestamp)
	}
	if tx.SenderPublicKey != senderPK {
		t.Errorf("Transaction SenderPublicKey = %s, want %s", tx.SenderPublicKey, senderPK)
	}
	if tx.Type != txType {
		t.Errorf("Transaction Type = %s, want %s", tx.Type, txType)
	}
	if !bytes.Equal(tx.Payload, payload) {
		t.Errorf("Transaction Payload = %v, want %v", tx.Payload, payload)
	}
	if tx.Signature != nil {
		t.Errorf("Transaction signature should be nil initially, got %x", tx.Signature)
	}

	// Test ID determinism
	tx2, _ := NewTransaction(senderPK, txType, payload)
	// Quick sleep to ensure timestamp *could* change if not handled well, though NewTransaction uses fixed ts for ID calc.
	// For this test, we're more interested if the ID calculation itself is stable for same inputs to HashTransactionContent.
	// The timestamp used for ID calculation is the one set *inside* NewTransaction.
	// To truly test determinism of ID given identical inputs to NewTransaction, we'd need to control time.
	// However, HashTransactionContent is tested for determinism separately.
	// Here we check if the ID is consistent for what NewTransaction produced.
	// The ID is based on the timestamp *at creation*. If we call it again, timestamp will be different.
	// So, we check if ID matches a re-calculation with its *own* timestamp.
	expectedID := HashTransactionContent(tx.Timestamp, tx.SenderPublicKey, tx.Type, tx.Payload)
	if tx.ID != expectedID {
		t.Errorf("Transaction ID is not consistent with its content hash. Got %s, expected %s", tx.ID, expectedID)
	}

	// Test error cases for NewTransaction
	_, err = NewTransaction("", txType, payload)
	if err == nil {
		t.Errorf("Expected error for empty sender public key, got nil")
	}
	_, err = NewTransaction(senderPK, "", payload)
	if err == nil {
		t.Errorf("Expected error for empty transaction type, got nil")
	}
}

func TestTransaction_SignAndVerifySignature_Placeholder(t *testing.T) {
	senderPK := "testSenderPKForSign"
	tx, _ := NewTransaction(senderPK, PostCreated, []byte("payload to sign"))

	// Test Sign
	var dummyPrivateKey []byte // Placeholder
	err := tx.Sign(dummyPrivateKey)
	if err != nil {
		t.Fatalf("tx.Sign() error = %v", err)
	}
	if len(tx.Signature) == 0 {
		t.Errorf("Transaction signature is empty after signing")
	}
	// Check if placeholder signature matches expectation from crypto_utils
	expectedSig, _ := SignData([]byte(tx.ID), dummyPrivateKey)
	if !bytes.Equal(tx.Signature, expectedSig) {
		t.Errorf("tx.Signature = %x, want %x", tx.Signature, expectedSig)
	}

	// Test VerifySignature
	valid, err := tx.VerifySignature()
	if err != nil {
		t.Fatalf("tx.VerifySignature() error = %v", err)
	}
	if !valid {
		t.Errorf("Expected signature to be valid, but was marked invalid")
	}

	// Tamper with signature
	originalSignature := make([]byte, len(tx.Signature))
	copy(originalSignature, tx.Signature)
	tx.Signature = []byte("tampered_signature_value")
	invalid, err := tx.VerifySignature()
	if err != nil { // Placeholder might not error on format, just content
		t.Logf("tx.VerifySignature() with tampered sig returned error (may be ok for placeholder): %v", err)
	}
	if invalid {
		t.Errorf("Expected tampered signature to be invalid, but was marked valid")
	}
	tx.Signature = originalSignature // Restore

	// Tamper with ID (data that was signed)
	originalID := tx.ID
	tx.ID = "tampered_id"
	invalidID, err := tx.VerifySignature()
	if err != nil {
		t.Logf("tx.VerifySignature() with tampered ID returned error: %v", err)
	}
	if invalidID {
		t.Errorf("Expected signature to be invalid with tampered ID, but was marked valid")
	}
	tx.ID = originalID // Restore
}

func TestTransaction_IsValid(t *testing.T) {
	validTx, _ := NewTransaction("sender", PostCreated, []byte("good payload"))
	_ = validTx.Sign([]byte("dummyKey")) // Sign it to make it more complete

	tests := []struct {
		name    string
		tx      *Transaction
		wantErr bool
	}{
		{"valid transaction", validTx, false},
		{
			"empty ID",
			&Transaction{ID: "", Timestamp: time.Now().UnixNano(), SenderPublicKey: "s", Type: PostCreated, Payload: []byte("p")},
			true,
		},
		{
			"invalid timestamp (zero)",
			&Transaction{ID: "id", Timestamp: 0, SenderPublicKey: "s", Type: PostCreated, Payload: []byte("p")},
			true,
		},
		{
			"invalid timestamp (negative)",
			&Transaction{ID: "id", Timestamp: -100, SenderPublicKey: "s", Type: PostCreated, Payload: []byte("p")},
			true,
		},
		{
			"empty sender public key",
			&Transaction{ID: "id", Timestamp: time.Now().UnixNano(), SenderPublicKey: "", Type: PostCreated, Payload: []byte("p")},
			true,
		},
		{
			"empty type",
			&Transaction{ID: "id", Timestamp: time.Now().UnixNano(), SenderPublicKey: "s", Type: "", Payload: []byte("p")},
			true,
		},
		// Note: tx.ID vs HashTransactionContent check was removed from IsValid as ID is set on creation.
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			err := tt.tx.IsValid()
			if (err != nil) != tt.wantErr {
				t.Errorf("Transaction.IsValid() error = %v, wantErr %v", err, tt.wantErr)
			}
		})
	}
}
