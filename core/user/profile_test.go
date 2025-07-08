package user

import (
	"encoding/json"
	"reflect"
	"testing"
	"time"
)

func TestNewProfile(t *testing.T) {
	owner := "ownerPubKey123"
	displayName := "Test User"
	bio := "A bio for testing."

	before := time.Now().UnixNano()
	profile := NewProfile(owner, displayName, bio)
	after := time.Now().UnixNano()

	if profile.OwnerPublicKey != owner {
		t.Errorf("NewProfile OwnerPublicKey = %s, want %s", profile.OwnerPublicKey, owner)
	}
	if profile.DisplayName != displayName {
		t.Errorf("NewProfile DisplayName = %s, want %s", profile.DisplayName, displayName)
	}
	if profile.Bio != bio {
		t.Errorf("NewProfile Bio = %s, want %s", profile.Bio, bio)
	}
	if profile.Timestamp < before || profile.Timestamp > after {
		t.Errorf("NewProfile Timestamp %d is out of expected range (%d-%d)", profile.Timestamp, before, after)
	}
	if profile.Version != 1 {
		t.Errorf("NewProfile Version = %d, want 1", profile.Version)
	}
	if profile.ProfilePictureCID != "" {
		t.Errorf("NewProfile ProfilePictureCID should be empty, got %s", profile.ProfilePictureCID)
	}
}

func TestProfile_Update(t *testing.T) {
	owner := "owner1"
	initialDisplayName := "Initial Name"
	initialBio := "Initial Bio"
	profile := NewProfile(owner, initialDisplayName, initialBio)
	initialTimestamp := profile.Timestamp
	initialVersion := profile.Version

	time.Sleep(1 * time.Nanosecond) // Ensure timestamp can change

	tests := []struct {
		name               string
		newDisplayName     string
		newBio             string
		newProfilePicCID   string
		newHeaderCID	   string
		wantChanged        bool
		expectedVersion    int
		expectedDisplayName string
		expectedBio        string
		expectedPicCID     string
		expectedHeaderCID  string
	}{
		{
			name: "update display name",
			newDisplayName: "Updated Name", newBio: initialBio, newProfilePicCID: "", newHeaderCID: "",
			wantChanged: true, expectedVersion: initialVersion + 1,
			expectedDisplayName: "Updated Name", expectedBio: initialBio, expectedPicCID: "", expectedHeaderCID: "",
		},
		{
			name: "update bio",
			newDisplayName: initialDisplayName, newBio: "Updated Bio", newProfilePicCID: "", newHeaderCID: "",
			wantChanged: true, expectedVersion: initialVersion + 1,
			expectedDisplayName: initialDisplayName, expectedBio: "Updated Bio", expectedPicCID: "", expectedHeaderCID: "",
		},
		{
			name: "update picture CID",
			newDisplayName: initialDisplayName, newBio: initialBio, newProfilePicCID: "newPicCID", newHeaderCID: "",
			wantChanged: true, expectedVersion: initialVersion + 1,
			expectedDisplayName: initialDisplayName, expectedBio: initialBio, expectedPicCID: "newPicCID", expectedHeaderCID: "",
		},
		{
			name: "update header CID",
			newDisplayName: initialDisplayName, newBio: initialBio, newProfilePicCID: "", newHeaderCID: "newHeaderCID",
			wantChanged: true, expectedVersion: initialVersion + 1,
			expectedDisplayName: initialDisplayName, expectedBio: initialBio, expectedPicCID: "", expectedHeaderCID: "newHeaderCID",
		},
		{
			name: "update all fields",
			newDisplayName: "All New Name", newBio: "All New Bio", newProfilePicCID: "allNewPic", newHeaderCID: "allNewHeader",
			wantChanged: true, expectedVersion: initialVersion + 1,
			expectedDisplayName: "All New Name", expectedBio: "All New Bio", expectedPicCID: "allNewPic", expectedHeaderCID: "allNewHeader",
		},
		{
			name: "no changes",
			newDisplayName: initialDisplayName, newBio: initialBio, newProfilePicCID: "", newHeaderCID: "",
			wantChanged: false, expectedVersion: initialVersion, // Version should not change
			expectedDisplayName: initialDisplayName, expectedBio: initialBio, expectedPicCID: "", expectedHeaderCID: "",
		},
		{
			name: "clear bio",
			newDisplayName: initialDisplayName, newBio: "", newProfilePicCID: "", newHeaderCID: "",
			wantChanged: true, expectedVersion: initialVersion + 1,
			expectedDisplayName: initialDisplayName, expectedBio: "", expectedPicCID: "", expectedHeaderCID: "",
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Reset profile for each test case to a known state
			p := NewProfile(owner, initialDisplayName, initialBio)
			p.Timestamp = initialTimestamp // Keep timestamp same for version check unless changed
			p.Version = initialVersion

			changed := p.Update(tt.newDisplayName, tt.newBio, tt.newProfilePicCID, tt.newHeaderCID)
			if changed != tt.wantChanged {
				t.Errorf("Profile.Update() changed = %v, want %v", changed, tt.wantChanged)
			}
			if p.Version != tt.expectedVersion {
				t.Errorf("Profile.Update() Version = %d, want %d", p.Version, tt.expectedVersion)
			}
			if changed && tt.wantChanged && p.Timestamp == initialTimestamp {
				t.Errorf("Profile.Update() Timestamp was not updated on change")
			}
			if !changed && !tt.wantChanged && p.Timestamp != initialTimestamp {
				t.Errorf("Profile.Update() Timestamp was updated but no change reported")
			}
			if p.DisplayName != tt.expectedDisplayName {
				t.Errorf("Profile.DisplayName = %s, want %s", p.DisplayName, tt.expectedDisplayName)
			}
			if p.Bio != tt.expectedBio {
				t.Errorf("Profile.Bio = %s, want %s", p.Bio, tt.expectedBio)
			}
			if p.ProfilePictureCID != tt.expectedPicCID {
				t.Errorf("Profile.ProfilePictureCID = %s, want %s", p.ProfilePictureCID, tt.expectedPicCID)
			}
			if p.HeaderImageCID != tt.expectedHeaderCID {
				t.Errorf("Profile.HeaderImageCID = %s, want %s", p.HeaderImageCID, tt.expectedHeaderCID)
			}
		})
	}
}

func TestProfile_ToJSON_And_ProfileFromJSON(t *testing.T) {
	originalProfile := NewProfile("owner123", "JSON User", "Loves JSON.")
	originalProfile.ProfilePictureCID = "jsonPicCID"
	originalProfile.HeaderImageCID = "jsonHeaderCID"
	originalProfile.Version = 5
	originalProfile.Timestamp = time.Now().UnixNano() // Ensure specific timestamp

	jsonData, err := originalProfile.ToJSON()
	if err != nil {
		t.Fatalf("Profile.ToJSON() error = %v", err)
	}
	if len(jsonData) == 0 {
		t.Fatal("Profile.ToJSON() returned empty data")
	}

	// Check if it's valid JSON (basic check)
	var temp map[string]interface{}
	if err := json.Unmarshal(jsonData, &temp); err != nil {
		t.Fatalf("Marshaled data is not valid JSON: %v\nData: %s", err, string(jsonData))
	}


	deserializedProfile, err := ProfileFromJSON(jsonData)
	if err != nil {
		t.Fatalf("ProfileFromJSON() error = %v\nData: %s", err, string(jsonData))
	}

	if !reflect.DeepEqual(originalProfile, deserializedProfile) {
		t.Errorf("Deserialized profile does not match original.\nOriginal: %+v\nDeserialized: %+v", originalProfile, deserializedProfile)
	}

	// Test error cases for ProfileFromJSON
	_, err = ProfileFromJSON([]byte("invalid json"))
	if err == nil {
		t.Error("ProfileFromJSON with invalid JSON: expected error, got nil")
	}
	_, err = ProfileFromJSON([]byte(`{"displayName": "Test"}`)) // Missing required fields
	if err == nil {
		t.Error("ProfileFromJSON with missing OwnerPublicKey: expected error, got nil")
	}
	_, err = ProfileFromJSON([]byte(`{"ownerPublicKey": "pk", "timestamp": 1, "version": 1}`)) // Missing display name
	if err == nil {
		t.Error("ProfileFromJSON with missing DisplayName: expected error, got nil")
	}
    _, err = ProfileFromJSON([]byte(`{"ownerPublicKey": "pk", "displayName":"dn", "timestamp": 0, "version": 1 }`))
    if err == nil {
        t.Error("ProfileFromJSON with zero timestamp: expected error, got nil")
    }
    _, err = ProfileFromJSON([]byte(`{"ownerPublicKey": "pk", "displayName":"dn", "timestamp": 123, "version": 0 }`))
    if err == nil {
        t.Error("ProfileFromJSON with invalid version: expected error, got nil")
    }
}
