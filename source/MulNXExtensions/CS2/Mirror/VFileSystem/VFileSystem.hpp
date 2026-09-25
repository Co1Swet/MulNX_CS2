#pragma once
#include <Intro/CSModuleBase.hpp>

class VFileSystem final :public CSModuleBase {
    enum SearchPathAdd_t {
        PATH_ADD_TO_HEAD,
        PATH_ADD_TO_TAIL,
        PATH_ADD_TO_TAIL_ATINDEX,
    };

    enum SearchPathPriority_t {
        SEARCH_PATH_PRIORITY_DEFAULT = 0,
        SEARCH_PATH_PRIORITY_LOOSE_FILES = 1,
        SEARCH_PATH_PRIORITY_VPK = 2,
    };

    using AddSearchPath_t = void(*)(void* pThisconst, const char* pPath, const char* pathID,
        SearchPathAdd_t addType, SearchPathPriority_t priority, int unk);

    MulNX::Memory::DllModule filesystem_stdio{};
    void* pGameVFileSystem = nullptr;

    bool Init()override;
public:
    void AddSearchPath(const char* pPath, const char* pathID,
        SearchPathAdd_t addType = PATH_ADD_TO_TAIL,
        SearchPathPriority_t priority = SEARCH_PATH_PRIORITY_DEFAULT, int unk = 0
    );
};