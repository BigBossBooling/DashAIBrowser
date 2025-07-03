// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASOL_CORE_CONFIG_LOADER_H_
#define ASOL_CORE_CONFIG_LOADER_H_

#include <string>

#include "base/files/file_path.h"

namespace asol {
namespace core {

// ConfigLoader provides utilities for loading ASOL configuration from files.
class ConfigLoader {
 public:
  // Load configuration from a file
  static std::string LoadFromFile(const base::FilePath& file_path);
  
  // Load configuration from the default location
  static std::string LoadDefault();
  
  // Get the default configuration file path
  static base::FilePath GetDefaultConfigPath();
};

}  // namespace core
}  // namespace asol

#endif  // ASOL_CORE_CONFIG_LOADER_H_