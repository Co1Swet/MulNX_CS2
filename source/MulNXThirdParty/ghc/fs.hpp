#pragma once
#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#define GHC_FILESYSTEM_ENFORCE_CPP17_API
#include "filesystem.hpp"
namespace fs = ghc::filesystem;