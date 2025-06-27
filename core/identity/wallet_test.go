package identity

import (
	"bytes"
	"digisocialblock/core/ledger" // Adjust import path as necessary
	"os"
	"testing"
)

func TestNewWallet(t *testing.T) {
	wallet, err := NewWallet()
	if err != nil {
		t.Fatalf("NewWallet() error = %v", err)
	}
	if wallet == nil {
		t.Fatal("NewWallet() returned nil")
	}
	if wallet.PrivateKey == nil {
		t.Errorf("Wallet.PrivateKey is nil")
	}
	if wallet.PublicKey == nil {
		t.Errorf("Wallet.PublicKey is nil")
	}
	if wallet.Address == "" {
		t.Errorf("Wallet.Address is empty")
	}

	// Check if address matches public key
	expectedAddr, _ := PublicKeyToAddress(wallet.PublicKey)
	if wallet.Address != expectedAddr {
		t.Errorf("Wallet.Address = %s, want %s (derived from PublicKey)", wallet.Address, expectedAddr)
	}
}

func TestWallet_GetAddress(t *testing.T) {
	wallet, _ := NewWallet()
	addr := wallet.GetAddress()
	if addr == "" {
		t.Errorf("GetAddress() returned empty string")
	}
	if addr != wallet.Address {
		t.Errorf("GetAddress() = %s, want %s (from wallet.Address field)", addr, wallet.Address)
	}
}

func TestWallet_SignAndVerifyTransaction(t *testing.T) {
	// Create a wallet
	wallet, err := NewWallet()
	if err != nil {
		t.Fatalf("Failed to create wallet for testing: %v", err)
	}

	// Create a sample transaction (using ledger package)
	// Note: Transaction.SenderPublicKey should match wallet.Address
	tx, err := ledger.NewTransaction(wallet.Address, ledger.PostCreated, []byte("test payload for signing"))
	if err != nil {
		t.Fatalf("Failed to create new transaction for testing: %v", err)
	}

	// Sign the transaction with the wallet
	err = wallet.SignTransaction(tx)
	if err != nil {
		t.Fatalf("wallet.SignTransaction() error = %v", err)
	}
	if len(tx.Signature) == 0 {
		t.Errorf("Transaction signature is empty after signing")
	}

	// Verify the signature on the transaction (using ledger.Transaction.VerifySignature)
	valid, err := tx.VerifySignature()
	if err != nil {
		t.Fatalf("tx.VerifySignature() error = %v", err)
	}
	if !valid {
		t.Errorf("Transaction signature verification failed for a valid signature")
	}

	// --- Test with tampered data ---
	// Scenario 1: Tamper with transaction payload (which changes its ID) after signing
	originalPayload := tx.Payload
	tx.Payload = []byte("tampered payload")
	// ID should ideally be recalculated if payload changes, but for this test,
	// we assume ID remains based on original payload, so signature won't match new content.
	// However, VerifySignature verifies against tx.ID. So, if ID is not updated,
	// verification *might* still pass if signature matched original ID.
	// A better test: keep tx.ID as is, but try to verify against a *different* hash.
	// For now, let's assume tx.ID is what was signed. If we change tx.ID itself:
	originalID := tx.ID
	tx.ID = "completely_different_id_hash"
	invalidTamperedID, errTamperID := tx.VerifySignature()
	if errTamperID == nil && invalidTamperedID { // Expecting verification to fail
		t.Errorf("Expected signature verification to fail for tampered ID, but it passed")
	}
	if errTamperID != nil {
		t.Logf("Signature verification for tampered ID correctly failed with error: %v", errTamperID)
	}
	tx.ID = originalID          // Restore ID
	tx.Payload = originalPayload // Restore payload

	// Scenario 2: Tamper with the signature itself
	originalSignature := make([]byte, len(tx.Signature))
	copy(originalSignature, tx.Signature)
	tx.Signature[0] ^= 0xff // Flip some bits in the signature
	invalidTamperedSig, errTamperSig := tx.VerifySignature()
	if errTamperSig == nil && invalidTamperedSig {
		t.Errorf("Expected signature verification to fail for tampered signature, but it passed")
	}
	if errTamperSig != nil {
		t.Logf("Signature verification for tampered signature correctly failed with error: %v", errTamperSig)
	}
	tx.Signature = originalSignature // Restore

	// Scenario 3: Try to verify with a different public key (by changing SenderPublicKey on tx)
	wallet2, _ := NewWallet()
	originalSenderPK := tx.SenderPublicKey
	tx.SenderPublicKey = wallet2.Address // Use a different public key

	invalidWrongKey, errWrongKey := tx.VerifySignature()
	if errWrongKey == nil && invalidWrongKey {
		t.Errorf("Expected signature verification to fail with wrong public key, but it passed")
	}
	if errWrongKey != nil {
		t.Logf("Signature verification with wrong public key correctly failed with error: %v", errWrongKey)
	}
	tx.SenderPublicKey = originalSenderPK // Restore
}


func TestWallet_Sign(t *testing.T) {
	wallet, _ := NewWallet()
	dataHash := ledger.CalculateSHA256Hash([]byte("some data to sign"))

	sig, err := wallet.Sign([]byte(dataHash))
	if err != nil {
		t.Fatalf("wallet.Sign() error = %v", err)
	}
	if len(sig) == 0 {
		t.Error("wallet.Sign() returned empty signature")
	}

	// Verify this signature directly using ecdsa.VerifyASN1 (as Transaction.VerifySignature would)
	// This is more of an integration check within the identity package itself.
	valid := ecdsa.VerifyASN1(wallet.PublicKey, []byte(dataHash), sig)
	if !valid {
		t.Errorf("ecdsa.VerifyASN1 failed for signature generated by wallet.Sign()")
	}
}


func TestWalletPersistence(t *testing.T) {
	// Create a temporary file for saving the wallet
	tmpFile, err := os.CreateTemp("", "testwallet_*.json")
	if err != nil {
		t.Fatalf("Failed to create temp file: %v", err)
	}
	filePath := tmpFile.Name()
	// Clean up the temp file after the test
	defer os.Remove(filePath)
	tmpFile.Close() // Close it as SaveToFile/LoadWalletFromFile will open/close

	// 1. Create a new wallet
	wallet1, err := NewWallet()
	if err != nil {
		t.Fatalf("NewWallet() failed: %v", err)
	}

	// 2. Save the wallet to file
	err = wallet1.SaveToFile(filePath)
	if err != nil {
		t.Fatalf("wallet1.SaveToFile(%s) error = %v", filePath, err)
	}

	// 3. Load the wallet from file
	wallet2, err := LoadWalletFromFile(filePath)
	if err != nil {
		t.Fatalf("LoadWalletFromFile(%s) error = %v", filePath, err)
	}

	// 4. Verify the loaded wallet is the same as the original
	if wallet2 == nil {
		t.Fatal("Loaded wallet is nil")
	}
	if !wallet1.PrivateKey.Equal(wallet2.PrivateKey) {
		t.Errorf("Loaded private key does not match original private key")
	}
	if !wallet1.PublicKey.Equal(wallet2.PublicKey) {
		t.Errorf("Loaded public key does not match original public key")
	}
	if wallet1.Address != wallet2.Address {
		t.Errorf("Loaded address = %s, want %s", wallet2.Address, wallet1.Address)
	}

	// Test loading a non-existent file
	_, err = LoadWalletFromFile("non_existent_wallet.json")
	if err == nil {
		t.Errorf("LoadWalletFromFile with non-existent file expected error, got nil")
	}

	// Test loading a corrupted/invalid file (simple case: empty file)
	emptyFile, _ := os.CreateTemp("", "emptywallet_*.json")
	emptyFilePath := emptyFile.Name()
	defer os.Remove(emptyFilePath)
	emptyFile.Close()

	_, err = LoadWalletFromFile(emptyFilePath)
	if err == nil {
		t.Errorf("LoadWalletFromFile with empty file expected error, got nil")
	}

	// Test loading a file with invalid hex for private key
	corruptedData := `{"privateKeyHex": "invalid-hex", "publicKeyHex": "somepubkeyhex", "address": "someaddress"}`
	corruptedFilePath := tmpFile.Name() + "_corrupt.json" // ensure different name for cleanup if needed
	err = os.WriteFile(corruptedFilePath, []byte(corruptedData), 0600)
	if err != nil {
		t.Fatalf("Failed to write corrupted wallet file: %v", err)
	}
	defer os.Remove(corruptedFilePath)
	_, err = LoadWalletFromFile(corruptedFilePath)
	if err == nil {
		t.Errorf("LoadWalletFromFile with corrupted hex data expected error, got nil")
	}

}

// Test that SignTransaction correctly updates the transaction's SenderPublicKey if it's empty.
func TestWallet_SignTransaction_SetsSenderPublicKey(t *testing.T) {
	wallet, _ := NewWallet()
	tx, _ := ledger.NewTransaction("", ledger.PostCreated, []byte("payload")) // Empty SenderPublicKey initially

	err := wallet.SignTransaction(tx)
	if err != nil {
		t.Fatalf("SignTransaction failed: %v", err)
	}
	if tx.SenderPublicKey != wallet.Address {
		t.Errorf("SignTransaction did not set SenderPublicKey. Got %s, expected %s", tx.SenderPublicKey, wallet.Address)
	}
}

// Test that SignTransaction returns an error if tx.SenderPublicKey is different from wallet's address.
func TestWallet_SignTransaction_MismatchedSenderPublicKey(t *testing.T) {
	wallet1, _ := NewWallet()
	wallet2, _ := NewWallet() // A different wallet

	tx, _ := ledger.NewTransaction(wallet2.Address, ledger.PostCreated, []byte("payload")) // Sender is wallet2

	err := wallet1.SignTransaction(tx) // Trying to sign with wallet1
	if err == nil {
		t.Errorf("Expected error when SignTransaction is called with mismatched SenderPublicKey, but got nil")
	}
}
