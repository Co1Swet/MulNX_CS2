#pragma once
#include <Intro/CSModuleBase.hpp>

class FileInjector final :public CSModuleBase {
    class VFileSystem* pVFileSystem = nullptr;
    bool Init()override;
};