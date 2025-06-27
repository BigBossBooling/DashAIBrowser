package user

import (
	"digisocialblock/core/content" // Path to content publisher/retriever
	"fmt"
	// "encoding/json" // Already used in profile.go, but here for clarity if needed directly
)

// ProfileManager handles the creation, updating, and retrieval of user profiles
// by interacting with the DDS content services.
type ProfileManager struct {
	publisher  *content.ContentPublisher // Service to publish content to DDS
	retriever  *content.ContentRetriever // Service to retrieve content from DDS
}

// NewProfileManager creates a new ProfileManager.
// It requires a ContentPublisher and ContentRetriever to interact with DDS.
func NewProfileManager(publisher *content.ContentPublisher, retriever *content.ContentRetriever) (*ProfileManager, error) {
	if publisher == nil {
		return nil, fmt.Errorf("content publisher cannot be nil")
	}
	if retriever == nil {
		return nil, fmt.Errorf("content retriever cannot be nil")
	}
	return &ProfileManager{
		publisher:  publisher,
		retriever:  retriever,
	}, nil
}

// PublishProfile serializes a Profile struct to JSON and publishes it to DDS.
// It returns the DDS Content ID (CID) of the published profile data.
func (pm *ProfileManager) PublishProfile(profileData *Profile) (string, error) {
	if profileData == nil {
		return "", fmt.Errorf("profile data cannot be nil")
	}

	// Ensure timestamp and version are set if it's a new profile being published this way
	// (though NewProfile already does this). This is more of a safeguard.
	if profileData.Timestamp == 0 {
		profileData.Timestamp = time.Now().UnixNano()
	}
	if profileData.Version == 0 {
		profileData.Version = 1
	}


	jsonData, err := profileData.ToJSON()
	if err != nil {
		return "", fmt.Errorf("failed to serialize profile to JSON for publishing: %w", err)
	}

	// Use the ContentPublisher to publish the JSON string (as text)
	// The ContentPublisher's PublishTextPostToDDS expects a string.
	profileCID, err := pm.publisher.PublishTextPostToDDS(string(jsonData))
	if err != nil {
		return "", fmt.Errorf("failed to publish profile JSON to DDS: %w", err)
	}

	return profileCID, nil
}

// RetrieveProfile fetches a user's profile from DDS using its Content ID (CID).
// It retrieves the JSON data, deserializes it into a Profile struct, and returns it.
func (pm *ProfileManager) RetrieveProfile(profileCID string) (*Profile, error) {
	if profileCID == "" {
		return nil, fmt.Errorf("profile CID cannot be empty")
	}

	// Use ContentRetriever to get the JSON string data
	profileJSONString, err := pm.retriever.RetrieveAndVerifyTextPost(profileCID)
	if err != nil {
		return nil, fmt.Errorf("failed to retrieve profile data from DDS for CID %s: %w", profileCID, err)
	}

	if profileJSONString == "" && profileCID != "empty_content_manifest_cid_v1" { // Allow empty string for specific empty CID
        // This might happen if RetrieveAndVerifyTextPost returns an empty string for non-empty CIDs
        // due to some internal logic or if the content was indeed an empty string.
        // ProfileFromJSON will likely fail on empty string if fields are required.
		return nil, fmt.Errorf("retrieved empty profile data from DDS for CID %s, but expected JSON", profileCID)
	}
    if profileJSONString == "" && profileCID == "empty_content_manifest_cid_v1" {
        // If it's the special CID for empty content, attempting to parse it as a Profile will fail.
        // This indicates the CID does not point to a valid profile.
        return nil, fmt.Errorf("CID %s points to empty content, not a valid profile JSON", profileCID)
    }


	profileData, err := ProfileFromJSON([]byte(profileJSONString))
	if err != nil {
		return nil, fmt.Errorf("failed to deserialize profile JSON (CID: %s): %w", profileCID, err)
	}

	return profileData, nil
}
