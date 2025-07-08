// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CORE_SECURITY_ZERO_KNOWLEDGE_SYNC_H_
#define BROWSER_CORE_SECURITY_ZERO_KNOWLEDGE_SYNC_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace browser_core {
namespace security {

// ZeroKnowledgeSync provides secure synchronization of browser data
// without the server being able to read the data.
class ZeroKnowledgeSync {
 public:
  // Data types that can be synced
  enum class DataType {
    BOOKMARKS,
    HISTORY,
    PASSWORDS,
    PREFERENCES,
    EXTENSIONS,
    OPEN_TABS,
    AUTOFILL,
    CUSTOM_DATA
  };

  // Sync status
  enum class SyncStatus {
    IDLE,
    SYNCING,
    ERROR,
    PAUSED
  };

  // Sync result
  struct SyncResult {
    bool success;
    std::string error_message;
    base::Time timestamp;
    std::unordered_map<DataType, int> items_synced;
  };

  // Sync item
  struct SyncItem {
    std::string id;
    DataType type;
    std::string encrypted_data;
    std::string metadata;
    base::Time last_modified;
    int version;
  };

  // Callback for sync operations
  using SyncCallback = base::OnceCallback<void(const SyncResult&)>;

  ZeroKnowledgeSync();
  ~ZeroKnowledgeSync();

  // Disallow copy and assign
  ZeroKnowledgeSync(const ZeroKnowledgeSync&) = delete;
  ZeroKnowledgeSync& operator=(const ZeroKnowledgeSync&) = delete;

  // Initialize the sync system
  bool Initialize();

  // Set up sync with account
  void SetupSync(const std::string& account_id, 
               const std::string& passphrase,
               SyncCallback callback);

  // Start syncing
  void StartSync(SyncCallback callback);

  // Stop syncing
  void StopSync();

  // Pause syncing
  void PauseSync();

  // Resume syncing
  void ResumeSync();

  // Get sync status
  SyncStatus GetSyncStatus() const;

  // Enable/disable syncing for a data type
  void EnableDataType(DataType type, bool enable);
  bool IsDataTypeEnabled(DataType type) const;

  // Get last sync time
  base::Time GetLastSyncTime() const;

  // Force sync now
  void SyncNow(SyncCallback callback);

  // Reset sync (clear all sync data)
  void ResetSync(base::OnceClosure callback);

  // Change sync passphrase
  void ChangePassphrase(const std::string& old_passphrase,
                      const std::string& new_passphrase,
                      base::OnceCallback<void(bool)> callback);

  // Export sync data
  void ExportSyncData(const std::string& passphrase,
                    base::OnceCallback<void(const std::string&)> callback);

  // Import sync data
  void ImportSyncData(const std::string& data,
                    const std::string& passphrase,
                    SyncCallback callback);

  // Get a weak pointer to this instance
  base::WeakPtr<ZeroKnowledgeSync> GetWeakPtr();

 private:
  // Helper methods
  std::string EncryptData(const std::string& data, const std::string& key);
  std::string DecryptData(const std::string& encrypted_data, const std::string& key);
  std::string DeriveKeyFromPassphrase(const std::string& passphrase, const std::string& salt);
  bool VerifyPassphrase(const std::string& passphrase);

  // Private implementation
  class Impl;
  std::unique_ptr<Impl> impl_;

  // For weak pointers
  base::WeakPtrFactory<ZeroKnowledgeSync> weak_ptr_factory_{this};
};

}  // namespace security
}  // namespace browser_core

#endif  // BROWSER_CORE_SECURITY_ZERO_KNOWLEDGE_SYNC_H_