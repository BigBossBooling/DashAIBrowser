package identity

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"encoding/hex"
	"io"
	"fmt"
)

// GetRandReader returns rand.Reader, useful for mocking in tests if needed,
// but primarily just to provide the io.Reader interface expected by ecdsa.Sign.
func GetRandReader() io.Reader {
	return rand.Reader
}

// GenerateECDSAKeyPair generates a new ECDSA private and public key pair
// using the P-256 elliptic curve.
func GenerateECDSAKeyPair() (*ecdsa.PrivateKey, *ecdsa.PublicKey, error) {
	privateKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to generate ECDSA key pair: %w", err)
	}
	return privateKey, &privateKey.PublicKey, nil
}

// --- Private Key Serialization ---

// PrivateKeyToBytes serializes an ECDSA private key to its byte representation
// using PKCS#8 encoding.
func PrivateKeyToBytes(priv *ecdsa.PrivateKey) ([]byte, error) {
	if priv == nil {
		return nil, fmt.Errorf("private key is nil")
	}
	derBytes, err := x509.MarshalPKCS8PrivateKey(priv)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal private key to PKCS#8: %w", err)
	}
	return derBytes, nil
}

// BytesToPrivateKey deserializes bytes (PKCS#8 encoded) into an ECDSA private key.
func BytesToPrivateKey(derBytes []byte) (*ecdsa.PrivateKey, error) {
	if len(derBytes) == 0 {
		return nil, fmt.Errorf("private key bytes are empty")
	}
	key, err := x509.ParsePKCS8PrivateKey(derBytes)
	if err != nil {
		return nil, fmt.Errorf("failed to parse PKCS#8 private key: %w", err)
	}
	ecdsaPrivKey, ok := key.(*ecdsa.PrivateKey)
	if !ok {
		return nil, fmt.Errorf("parsed key is not an ECDSA private key")
	}
	return ecdsaPrivKey, nil
}

// PrivateKeyToHexString converts an ECDSA private key to a hex string.
func PrivateKeyToHexString(priv *ecdsa.PrivateKey) (string, error) {
	bytes, err := PrivateKeyToBytes(priv)
	if err != nil {
		return "", err
	}
	return hex.EncodeToString(bytes), nil
}

// HexStringToPrivateKey converts a hex string back to an ECDSA private key.
func HexStringToPrivateKey(hexStr string) (*ecdsa.PrivateKey, error) {
	bytes, err := hex.DecodeString(hexStr)
	if err != nil {
		return nil, fmt.Errorf("failed to decode hex string for private key: %w", err)
	}
	return BytesToPrivateKey(bytes)
}


// --- Public Key Serialization & Address ---

// PublicKeyToBytes serializes an ECDSA public key to its byte representation
// using PKIX encoding (SubjectPublicKeyInfo).
func PublicKeyToBytes(pub *ecdsa.PublicKey) ([]byte, error) {
	if pub == nil {
		return nil, fmt.Errorf("public key is nil")
	}
	derBytes, err := x509.MarshalPKIXPublicKey(pub)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal public key to PKIX: %w", err)
	}
	return derBytes, nil
}

// BytesToPublicKey deserializes bytes (PKIX encoded) into an ECDSA public key.
func BytesToPublicKey(derBytes []byte) (*ecdsa.PublicKey, error) {
	if len(derBytes) == 0 {
		return nil, fmt.Errorf("public key bytes are empty")
	}
	key, err := x509.ParsePKIXPublicKey(derBytes)
	if err != nil {
		return nil, fmt.Errorf("failed to parse PKIX public key: %w", err)
	}
	ecdsaPubKey, ok := key.(*ecdsa.PublicKey)
	if !ok {
		return nil, fmt.Errorf("parsed key is not an ECDSA public key")
	}
	return ecdsaPubKey, nil
}

// PublicKeyToAddress converts an ECDSA public key to a hex-encoded string address.
// This uses the marshaled PKIX representation of the public key.
// For use in Transaction.SenderPublicKey.
func PublicKeyToAddress(publicKey *ecdsa.PublicKey) (string, error) {
	if publicKey == nil {
		return "", fmt.Errorf("public key is nil")
	}
	pubKeyBytes, err := PublicKeyToBytes(publicKey)
	if err != nil {
		return "", fmt.Errorf("failed to convert public key to bytes for address: %w", err)
	}
	return hex.EncodeToString(pubKeyBytes), nil
}

// AddressToPublicKey converts a hex-encoded string address back to an ECDSA public key.
func AddressToPublicKey(addressHex string) (*ecdsa.PublicKey, error) {
	if addressHex == "" {
		return nil, fmt.Errorf("address string is empty")
	}
	pubKeyBytes, err := hex.DecodeString(addressHex)
	if err != nil {
		return nil, fmt.Errorf("failed to decode hex string for public key address: %w", err)
	}
	return BytesToPublicKey(pubKeyBytes)
}
