#include "DLLLoadDispatcher.hpp"

bool DLLLoadDispatcher::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW", [this](MulNX::Message& msg) {this->OnModuleLoaded(msg); });
    
    return true;
}

void DLLLoadDispatcher::OnModuleLoaded(MulNX::Message& msg) {
    auto path = msg.p1.as<LPCWSTR>();
    std::filesystem::path fsPath(path);
    std::unique_lock lock(this->smutex);
    if (this->loadedModules.find(fsPath) == this->loadedModules.end()) {
        this->loadedModules.insert(fsPath);
        lock.unlock();
        this->DispatchModuleLoadMessage(fsPath);
    }
}

void DLLLoadDispatcher::DispatchModuleLoadMessage(const std::filesystem::path& modulePath) {
    if (modulePath.filename() == L"client.dll") {
        this->PublishSync("Hook/LoadLibraryExW/client.dll"_hash);
    }
    if (modulePath.filename() == L"engine2.dll") {
        this->PublishSync("Hook/LoadLibraryExW/engine2.dll"_hash);
    }
    if (modulePath.filename() == L"tier0.dll") {
        this->PublishSync("Hook/LoadLibraryExW/tier0.dll"_hash);
    }
    if (modulePath.filename() == L"panorama.dll") {
        this->PublishSync("Hook/LoadLibraryExW/panorama.dll"_hash);
    }
    if (modulePath.filename() == L"d3d11.dll") {
        this->PublishSync("Hook/LoadLibraryExW/d3d11.dll"_hash);
    }
}