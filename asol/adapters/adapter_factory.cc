// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/adapters/adapter_interface.h"
#include "asol/adapters/gemini/gemini_text_adapter.h"

#include <memory>
#include <string>

#include "base/logging.h"

namespace asol {
namespace adapters {

// Factory function to create adapters by type
std::unique_ptr<AdapterInterface> CreateAdapter(const std::string& adapter_type) {
  if (adapter_type == "gemini") {
    return std::make_unique<gemini::GeminiTextAdapter>();
  }
  
  // Add more adapter types here as they are implemented
  
  LOG(ERROR) << "Unknown adapter type: " << adapter_type;
  return nullptr;
}

}  // namespace adapters
}  // namespace asol