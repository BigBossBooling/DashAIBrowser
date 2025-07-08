package user

import (
	"encoding/json"
	"fmt"
	"time"
)

// Profile represents a user's profile data.
type Profile struct {
	OwnerPublicKey    string `json:"ownerPublicKey"`    // Hex-encoded public key of the profile owner (for association)
	DisplayName       string `json:"displayName"`       // User's display name
	Bio               string `json:"bio,omitempty"`     // User's biography, optional
	ProfilePictureCID string `json:"profilePictureCID,omitempty"` // CID of the profile picture on DDS, optional
	HeaderImageCID    string `json:"headerImageCID,omitempty"`    // CID of a header/banner image on DDS, optional
	Timestamp         int64  `json:"timestamp"`         // UnixNano timestamp of when this profile version was created/updated
	Version           int    `json:"version"`           // Version number of the profile, incremented on updates
	// CustomFields map[string]string `json:"customFields,omitempty"` // For future extensibility
}

// NewProfile creates a new Profile instance.
// ownerPublicKey is the hex-encoded public key string of the user who owns this profile.
func NewProfile(ownerPublicKey, displayName, bio string) *Profile {
	return &Profile{
		OwnerPublicKey: ownerPublicKey,
		DisplayName:    displayName,
		Bio:            bio,
		Timestamp:      time.Now().UnixNano(),
		Version:        1, // Initial version
	}
}

// Update updates the profile with new data and increments the version.
// Fields that are empty in newProfileData are not updated, allowing partial updates.
// Returns true if any field was actually changed.
func (p *Profile) Update(newDisplayName, newBio, newProfilePicCID, newHeaderCID string) bool {
	changed := false
	if newDisplayName != "" && p.DisplayName != newDisplayName {
		p.DisplayName = newDisplayName
		changed = true
	}
	if newBio != "" && p.Bio != newBio { // Allow setting bio to empty if newBio is explicitly ""
		p.Bio = newBio
		changed = true
	} else if newBio == "" && p.Bio != "" { // Clearing a bio
        p.Bio = ""
        changed = true
    }

	if newProfilePicCID != "" && p.ProfilePictureCID != newProfilePicCID {
		p.ProfilePictureCID = newProfilePicCID
		changed = true
	}
    if newHeaderCID != "" && p.HeaderImageCID != newHeaderCID {
        p.HeaderImageCID = newHeaderCID
        changed = true
    }

	if changed {
		p.Timestamp = time.Now().UnixNano()
		p.Version++
	}
	return changed
}

// ToJSON serializes the Profile struct to a JSON byte slice.
func (p *Profile) ToJSON() ([]byte, error) {
	jsonData, err := json.MarshalIndent(p, "", "  ")
	if err != nil {
		return nil, fmt.Errorf("failed to marshal profile to JSON: %w", err)
	}
	return jsonData, nil
}

// FromJSON deserializes a JSON byte slice into a Profile struct.
func ProfileFromJSON(jsonData []byte) (*Profile, error) {
	var p Profile
	err := json.Unmarshal(jsonData, &p)
	if err != nil {
		return nil, fmt.Errorf("failed to unmarshal JSON to profile: %w", err)
	}
	// Basic validation after unmarshal
	if p.OwnerPublicKey == "" {
		return nil, fmt.Errorf("unmarshaled profile has empty OwnerPublicKey")
	}
	if p.DisplayName == "" && p.Version > 0 { // Allow display name to be initially empty for a new profile perhaps, but not if versioned
		// This rule might be too strict, depends on application logic.
		// For now, let's say display name is required.
		return nil, fmt.Errorf("unmarshaled profile has empty DisplayName")
	}
	if p.Timestamp == 0 {
		return nil, fmt.Errorf("unmarshaled profile has zero timestamp")
	}
	if p.Version <= 0 {
		return nil, fmt.Errorf("unmarshaled profile has invalid version: %d", p.Version)
	}
	return &p, nil
}
