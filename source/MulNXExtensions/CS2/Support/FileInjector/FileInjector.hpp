#pragma once
#include <Intro/Intros.hpp>

class FileInjector final :public CSModuleBase {
    class VFileSystem* pVFileSystem = nullptr;
    bool Init()override;
};