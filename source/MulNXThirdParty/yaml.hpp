#pragma once
#include <yaml-cpp/yaml.h>
#include <MulNXThirdParty/ghc/fs.hpp>

namespace YAML {
    inline Node LoadFilePath(const ghc::filesystem::path& path) {
        ghc::filesystem::ifstream fin(path);
        if (!fin) {
            throw BadFile(path.string());   // UTF-8 模式下是 UTF-8 字符串
        }
        return Load(fin);
    }
}