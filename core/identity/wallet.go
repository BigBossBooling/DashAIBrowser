package identity

import (
	"crypto/ecdsa"
	"crypto/rand" // For ecdsa.Sign
	"digisocialblock/core/ledger" // Assuming this path based on previous structure
	"fmt"
	// "math/big" // Required for ecdsa.Sign Ecdsa signatures are a pair of integers (r, s).
)

// Wallet holds a user's cryptographic key pair and their public address.
type Wallet struct {
	PrivateKey *ecdsa.PrivateKey
	PublicKey  *ecdsa.PublicKey
	Address    string // Derived from PublicKey, typically hex-encoded
}

// NewWallet creates a new Wallet instance, generating a new ECDSA key pair.
func NewWallet() (*Wallet, error) {
	privKey, pubKey, err := GenerateECDSAKeyPair()
	if err != nil {
		return nil, fmt.Errorf("failed to generate key pair for new wallet: %w", err)
	}

	address, err := PublicKeyToAddress(pubKey)
	if err != nil {
		return nil, fmt.Errorf("failed to derive address for new wallet: %w", err)
	}

	return &Wallet{
		PrivateKey: privKey,
		PublicKey:  pubKey,
		Address:    address,
	}, nil
}

// GetAddress returns the public address of the wallet.
func (w *Wallet) GetAddress() string {
	return w.Address
}

// Sign uses the wallet's private key to sign a hash (typically a transaction ID).
// Returns the ASN.1 DER encoded signature.
func (w *Wallet) Sign(dataHash []byte) ([]byte, error) {
	if w.PrivateKey == nil {
		return nil, fmt.Errorf("wallet has no private key to sign with")
	}
	if len(dataHash) == 0 {
		return nil, fmt.Errorf("cannot sign empty data hash")
	}

	// ecdsa.SignASN1 signs a hash (which should be the output of a hash function)
	// and returns the ASN.1 DER encoded signature.
	signature, err := ecdsa.SignASN1(rand.Reader, w.PrivateKey, dataHash)
	if err != nil {
		return nil, fmt.Errorf("failed to sign data: %w", err)
	}
	return signature, nil
}

// SignTransaction signs a ledger transaction using the wallet's private key.
// It populates the transaction's Signature field.
// The transaction's ID field (which is a hash of its content) is what gets signed.
func (w *Wallet) SignTransaction(tx *ledger.Transaction) error {
	if tx == nil {
		return fmt.Errorf("cannot sign a nil transaction")
	}
	if tx.ID == "" {
		return fmt.Errorf("transaction ID is empty, cannot determine data to sign")
	}

	dataToSign := []byte(tx.ID) // The transaction ID (hash of content) is what we sign
	signature, err := w.Sign(dataToSign)
	if err != nil {
		return fmt.Errorf("failed to sign transaction ID %s: %w", tx.ID, err)
	}

	tx.Signature = signature
	// The SenderPublicKey should already be set on the transaction before signing,
	// and it should match this wallet's public key address.
	// For consistency, we can set it here if it's not already set, or verify it.
	if tx.SenderPublicKey == "" {
		tx.SenderPublicKey = w.Address
	} else if tx.SenderPublicKey != w.Address {
		return fmt.Errorf("transaction SenderPublicKey %s does not match wallet address %s", tx.SenderPublicKey, w.Address)
	}

	return nil
}

// --- Persistence (Placeholder for now, as per Task 1.1.6 for Blockchain) ---
// For Wallet, simple JSON or Gob encoding of the hex/base64 private key could be done.
// IMPORTANT: Real wallet persistence MUST encrypt the private key.

// WalletData is a helper struct for serializing/deserializing wallet data.
// We store the private key as a hex string for simplicity in JSON/Gob.
type WalletData struct {
	PrivateKeyHex string `json:"privateKeyHex"`
	PublicKeyHex  string `json:"publicKeyHex"` // Store for convenience/verification
	Address       string `json:"address"`
}

// SaveToFile serializes the wallet's private key to a file.
// NOTE: This is a placeholder and does NOT encrypt the private key.
// In a real application, the private key MUST be encrypted.
func (w *Wallet) SaveToFile(filepath string) error {
	// TODO: Implement proper encryption for the private key before saving.
	// For this task, we'll save it as hex for simplicity of demonstration.
	privKeyHex, err := PrivateKeyToHexString(w.PrivateKey)
	if err != nil {
		return fmt.Errorf("failed to convert private key to hex for saving: %w", err)
	}
	pubKeyHex, err := PublicKeyToAddress(w.PublicKey) // Address is already hex of public key
	if err != nil {
		return fmt.Errorf("failed to convert public key to hex for saving: %w", err)
	}


	data := WalletData{
		PrivateKeyHex: privKeyHex,
		PublicKeyHex:  pubKeyHex, // Same as w.Address if using PublicKeyToAddress
		Address:       w.Address,
	}

	// Using JSON for simplicity. Gob would also work.
	// jsonData, err := json.MarshalIndent(data, "", "  ")
	// if err != nil {
	// 	return fmt.Errorf("failed to marshal wallet data to JSON: %w", err)
	// }
	// return os.WriteFile(filepath, jsonData, 0600) // 0600 for private key file

	// For this task, we will use JSON encoding.
	// IMPORTANT: In a real application, the private key MUST be encrypted before saving.
	// This implementation does not include encryption.
	jsonData, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal wallet data to JSON: %w", err)
	}

	// TODO: Add a comment about file permissions for private key files (e.g., 0600)
	err = os.WriteFile(filepath, jsonData, 0600) // Restrictive permissions
	if err != nil {
		return fmt.Errorf("failed to write wallet file %s: %w", filepath, err)
	}
	fmt.Printf("Wallet for address %s saved to %s (UNENCRYPTED - FOR DEMO ONLY)\n", w.Address, filepath)
	return nil
}

// LoadWalletFromFile loads a wallet from a JSON file.
// NOTE: This implementation assumes the private key in the file is NOT encrypted.
func LoadWalletFromFile(filepath string) (*Wallet, error) {
	// TODO: Implement proper decryption if the private key was encrypted during save.
	fileData, err := os.ReadFile(filepath)
	if err != nil {
		return nil, fmt.Errorf("failed to read wallet file %s: %w", filepath, err)
	}

	var data WalletData
	if err := json.Unmarshal(fileData, &data); err != nil {
		return nil, fmt.Errorf("failed to unmarshal wallet data from JSON: %w", err)
	}

	privKey, err := HexStringToPrivateKey(data.PrivateKeyHex)
	if err != nil {
		return nil, fmt.Errorf("failed to convert hex to private key: %w", err)
	}

	// Reconstruct public key from private key for verification and to ensure consistency
	// Alternatively, could load PublicKeyHex and verify it matches.
	// For robustness, deriving from private key is good.
	reconstructedPubKey := &privKey.PublicKey

	// Verify the loaded address matches the address derived from the reconstructed public key
	addressFromLoadedKey, err := PublicKeyToAddress(reconstructedPubKey)
	if err != nil {
		return nil, fmt.Errorf("failed to derive address from loaded public key: %w", err)
	}
	if data.Address != addressFromLoadedKey {
		// Also check against PublicKeyHex if it was different
		if data.PublicKeyHex != "" {
			addrFromHexPub, _ := AddressToPublicKey(data.PublicKeyHex)
			if !reconstructedPubKey.Equal(addrFromHexPub) {
				 return nil, fmt.Errorf("loaded public key hex %s does not match private key's public key", data.PublicKeyHex)
			}
		}
		// If PublicKeyHex matched or was empty, but Address doesn't match derived, it's an issue.
		if data.Address != addressFromLoadedKey { // Re-check after potentially verifying PublicKeyHex
			return nil, fmt.Errorf("loaded address %s does not match address derived from private key %s", data.Address, addressFromLoadedKey)
		}
	}


	return &Wallet{
		PrivateKey: privKey,
		PublicKey:  reconstructedPubKey,
		Address:    data.Address, // Use the validated address from file
	}, nil
}
