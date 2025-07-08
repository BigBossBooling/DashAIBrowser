package identity

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"encoding/hex"
	"reflect"
	"testing"
)

func TestGenerateECDSAKeyPair(t *testing.T) {
	priv, pub, err := GenerateECDSAKeyPair()
	if err != nil {
		t.Fatalf("GenerateECDSAKeyPair() error = %v", err)
	}
	if priv == nil {
		t.Errorf("GenerateECDSAKeyPair() private key is nil")
	}
	if pub == nil {
		t.Errorf("GenerateECDSAKeyPair() public key is nil")
	}

	// Basic check: public key should correspond to private key
	if !pub.Equal(&priv.PublicKey) {
		t.Errorf("Generated public key does not match private key's public part")
	}

	// Check curve (should be P256)
	if pub.Curve != elliptic.P256() {
		t.Errorf("Generated key is not on P256 curve, got %v", pub.Curve)
	}
}

func TestPrivateKeySerialization(t *testing.T) {
	priv1, _, err := GenerateECDSAKeyPair()
	if err != nil {
		t.Fatalf("Failed to generate key pair for test: %v", err)
	}

	// Test ToBytes and FromBytes
	priv1Bytes, err := PrivateKeyToBytes(priv1)
	if err != nil {
		t.Fatalf("PrivateKeyToBytes() error = %v", err)
	}
	if len(priv1Bytes) == 0 {
		t.Errorf("PrivateKeyToBytes() returned empty byte slice")
	}

	priv2, err := BytesToPrivateKey(priv1Bytes)
	if err != nil {
		t.Fatalf("BytesToPrivateKey() error = %v", err)
	}
	if !priv1.Equal(priv2) { // Equal method for ecdsa.PrivateKey compares D and Curve parameters
		t.Errorf("Deserialized private key does not match original")
	}

	// Test ToHexString and FromHexString
	priv1Hex, err := PrivateKeyToHexString(priv1)
	if err != nil {
		t.Fatalf("PrivateKeyToHexString() error = %v", err)
	}
	if priv1Hex == "" {
		t.Errorf("PrivateKeyToHexString() returned empty string")
	}

	priv3, err := HexStringToPrivateKey(priv1Hex)
	if err != nil {
		t.Fatalf("HexStringToPrivateKey() error = %v", err)
	}
	if !priv1.Equal(priv3) {
		t.Errorf("Deserialized private key from hex does not match original")
	}

	// Test nil cases
	_, err = PrivateKeyToBytes(nil)
	if err == nil {
		t.Errorf("PrivateKeyToBytes(nil) expected error, got nil")
	}
	_, err = BytesToPrivateKey(nil)
	if err == nil {
		t.Errorf("BytesToPrivateKey(nil) expected error, got nil")
	}
	_, err = BytesToPrivateKey([]byte("invalid"))
	if err == nil {
		t.Errorf("BytesToPrivateKey with invalid bytes expected error, got nil")
	}
}

func TestPublicKeySerializationAndAddress(t *testing.T) {
	_, pub1, err := GenerateECDSAKeyPair()
	if err != nil {
		t.Fatalf("Failed to generate key pair for test: %v", err)
	}

	// Test ToBytes and FromBytes for Public Key
	pub1Bytes, err := PublicKeyToBytes(pub1)
	if err != nil {
		t.Fatalf("PublicKeyToBytes() error = %v", err)
	}
	if len(pub1Bytes) == 0 {
		t.Errorf("PublicKeyToBytes() returned empty byte slice")
	}

	pub2, err := BytesToPublicKey(pub1Bytes)
	if err != nil {
		t.Fatalf("BytesToPublicKey() error = %v", err)
	}
	if !pub1.Equal(pub2) {
		t.Errorf("Deserialized public key does not match original")
	}

	// Test PublicKeyToAddress and AddressToPublicKey
	address1, err := PublicKeyToAddress(pub1)
	if err != nil {
		t.Fatalf("PublicKeyToAddress() error = %v", err)
	}
	if address1 == "" {
		t.Errorf("PublicKeyToAddress() returned empty string")
	}

	// Verify address is hex encoding of pub1Bytes
	expectedAddress := hex.EncodeToString(pub1Bytes)
	if address1 != expectedAddress {
		t.Errorf("PublicKeyToAddress() = %s, want %s", address1, expectedAddress)
	}

	pub3, err := AddressToPublicKey(address1)
	if err != nil {
		t.Fatalf("AddressToPublicKey() error = %v", err)
	}
	if !pub1.Equal(pub3) {
		t.Errorf("Deserialized public key from address does not match original")
	}

	// Test nil/empty cases
	_, err = PublicKeyToBytes(nil)
	if err == nil {
		t.Errorf("PublicKeyToBytes(nil) expected error, got nil")
	}
	_, err = BytesToPublicKey(nil)
	if err == nil {
		t.Errorf("BytesToPublicKey(nil) expected error, got nil")
	}
	_, err = BytesToPublicKey([]byte("invalid"))
	if err == nil {
		t.Errorf("BytesToPublicKey with invalid bytes expected error, got nil")
	}
	_, err = PublicKeyToAddress(nil)
	if err == nil {
		t.Errorf("PublicKeyToAddress(nil) expected error, got nil")
	}
	_, err = AddressToPublicKey("")
	if err == nil {
		t.Errorf("AddressToPublicKey with empty string expected error, got nil")
	}
	_, err = AddressToPublicKey("invalidhex")
	if err == nil {
		t.Errorf("AddressToPublicKey with invalid hex expected error, got nil")
	}
}

func TestGetRandReader(t *testing.T) {
	reader := GetRandReader()
	if reader == nil {
		t.Fatal("GetRandReader returned nil")
	}
	// Check if it's the type we expect (crypto/rand.Reader)
	// This is a bit of a type assertion check.
	expectedType := reflect.TypeOf(rand.Reader)
	if reflect.TypeOf(reader) != expectedType {
		t.Errorf("GetRandReader returned type %v, want %v", reflect.TypeOf(reader), expectedType)
	}
}
