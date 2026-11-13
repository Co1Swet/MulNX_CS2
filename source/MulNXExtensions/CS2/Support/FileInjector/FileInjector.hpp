#pragma once
#include <Intro/Intros.hpp>

class FileInjector final :public CSModule<FileInjector> {
    class VFileSystem* pVFileSystem = nullptr;
    bool Init()override;
};