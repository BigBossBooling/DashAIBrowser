package content

import (
	"bytes"
	"digisocialblock/pkg/dds/chunking" // Assuming this path for your DDS packages
	"digisocialblock/pkg/dds/storage"   // Assuming this path
	// "digisocialblock/pkg/dds/originator" // Will be conceptual for now
	"fmt"
	"io"
	"log" // For logging conceptual originator call
)

// DDSStorage defines the interface for storing chunks.
// This should match the interface provided by your pkg/dds/storage package.
type DDSStorage interface {
	StoreChunk(chunkID string, data []byte) error
	RetrieveChunk(chunkID string) ([]byte, error)
	ChunkExists(chunkID string) bool
}

// DDSChunker defines the interface for chunking data.
// This should match the interface provided by your pkg/dds/chunking package.
type DDSChunker interface {
	ChunkData(data io.Reader) (*chunking.ContentManifestV1, []chunking.DataChunk, error)
}

// OriginatorAdversiter defines the interface for advertising content.
// Conceptual for now.
type OriginatorAdvertiser interface {
    AdvertiseManifest(manifest *chunking.ContentManifestV1) error
}

// SimplePlaceholderOriginator is a placeholder for DDS Originator functionality.
type SimplePlaceholderOriginator struct{}

func (spo *SimplePlaceholderOriginator) AdvertiseManifest(manifest *chunking.ContentManifestV1) error {
	log.Printf("Conceptual Originator: Advertising manifest with CID %s (Total Size: %d, Chunks: %d)\n",
		manifest.ManifestCID, manifest.TotalSize, len(manifest.Chunks))
	// In a real implementation, this would involve:
	// - Announcing the manifest to a DHT or other discovery mechanism.
	// - Potentially seeding the content to initial peers.
	return nil
}


// ContentPublisher service for publishing content to DDS.
type ContentPublisher struct {
	chunker   DDSChunker
	storage   DDSStorage
	originator OriginatorAdvertiser // Conceptual for now
}

// NewContentPublisher creates a new ContentPublisher.
func NewContentPublisher(chunker DDSChunker, store DDSStorage, originator OriginatorAdvertiser) (*ContentPublisher, error) {
	if chunker == nil {
		return nil, fmt.Errorf("chunker cannot be nil")
	}
	if store == nil {
		return nil, fmt.Errorf("storage cannot be nil")
	}
	if originator == nil {
		return nil, fmt.Errorf("originator cannot be nil")
	}
	return &ContentPublisher{
		chunker:   chunker,
		storage:   store,
		originator: originator,
	}, nil
}

// PublishTextPostToDDS chunks a text post, stores its chunks,
// conceptually advertises it, and returns the manifest CID.
func (cp *ContentPublisher) PublishTextPostToDDS(text string) (string, error) {
	if text == "" {
		return "", fmt.Errorf("cannot publish empty text content")
	}

	// 1. Chunk the data
	reader := bytes.NewReader([]byte(text))
	manifest, dataChunks, err := cp.chunker.ChunkData(reader)
	if err != nil {
		return "", fmt.Errorf("failed to chunk data: %w", err)
	}
	if manifest == nil || manifest.ManifestCID == "" {
		return "", fmt.Errorf("chunking produced an invalid or empty manifest CID")
	}

	fmt.Printf("ContentPublisher: Content chunked. Manifest CID: %s, Number of chunks: %d\n", manifest.ManifestCID, len(dataChunks))

	// 2. Store the chunks
	for _, chunk := range dataChunks {
		// Check if chunk already exists to avoid re-storing (optional, storage might handle this)
		// if cp.storage.ChunkExists(chunk.ChunkCID) {
		//  fmt.Printf("ContentPublisher: Chunk %s already exists, skipping store.\n", chunk.ChunkCID)
		//  continue
		// }
		err := cp.storage.StoreChunk(chunk.ChunkCID, chunk.Data)
		if err != nil {
			// TODO: Add rollback logic for partially stored chunks if a later chunk fails?
			// For now, fail fast.
			return "", fmt.Errorf("failed to store chunk %s: %w", chunk.ChunkCID, err)
		}
		// fmt.Printf("ContentPublisher: Stored chunk %s\n", chunk.ChunkCID)
	}
	fmt.Printf("ContentPublisher: All %d chunks stored successfully.\n", len(dataChunks))

	// 3. (Conceptual) Advertise content via Originator
	if err := cp.originator.AdvertiseManifest(manifest); err != nil {
		// This is conceptual, so error handling might be just logging for now.
		// In a real system, failure to advertise might be critical.
		log.Printf("ContentPublisher: Warning - failed to advertise manifest %s: %v\n", manifest.ManifestCID, err)
		// Depending on requirements, might not return an error for advertisement failure if local storage succeeded.
	}


	return manifest.ManifestCID, nil
}
