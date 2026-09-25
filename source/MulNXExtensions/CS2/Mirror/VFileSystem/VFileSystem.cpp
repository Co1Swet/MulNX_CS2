#include "VFileSystem.hpp"

bool VFileSystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/filesystem_stdio.dll", [this](auto&&...) {
        this->filesystem_stdio = MulNX::Memory::DllModule(L"filesystem_stdio.dll");
        this->pGameVFileSystem = (void*)(this->filesystem_stdio.GetBaseAddress() +
            cs2_dumper::interfaces::filesystem_stdio_dll::VFileSystem017);
        });

    return true;
}

void VFileSystem::AddSearchPath(const char* pPath, const char* pathID,
    SearchPathAdd_t addType, SearchPathPriority_t priority, int unk) {
    auto pF = (AddSearchPath_t)IVClass::Assume(this->pGameVFileSystem)->GetVFuncPtr(31);
    pF(this->pGameVFileSystem, pPath, pathID, addType, priority, unk);
}