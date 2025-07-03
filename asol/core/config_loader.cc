// Copyright 2025 The DashAIBrowser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "asol/core/config_loader.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"

namespace asol {
namespace core {

std::string ConfigLoader::LoadFromFile(const base::FilePath& file_path) {
  std::string config_json;
  if (!base::ReadFileToString(file_path, &config_json)) {
    LOG(ERROR) << "Failed to read configuration file: " << file_path;
    return "{}";  // Return empty JSON object on failure
  }
  
  return config_json;
}

std::string ConfigLoader::LoadDefault() {
  return LoadFromFile(GetDefaultConfigPath());
}

base::FilePath ConfigLoader::GetDefaultConfigPath() {
  base::FilePath exe_dir;
  if (!base::PathService::Get(base::DIR_EXE, &exe_dir)) {
    LOG(ERROR) << "Failed to get executable directory";
    return base::FilePath();
  }
  
  return exe_dir.Append("asol_config.json");
}

}  // namespace core
}  // namespace asol