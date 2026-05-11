#include "DLLLoadDispatcher.hpp"

bool DLLLoadDispatcher::Init() {
    this->ISys()
        .SubscribeSync("Hook/LoadLibraryExW", [this](MulNX::Message& msg) {this->OnModuleLoaded(msg); });
    
    return true;
}

void DLLLoadDispatcher::OnModuleLoaded(MulNX::Message& msg) {
    auto path = msg.p1.as<LPCWSTR>();
    std::filesystem::path fsPath(path);
    if (this->loadedModules.find(fsPath) == this->loadedModules.end()) {
        this->loadedModules.insert(fsPath);
        this->DispatchModuleLoadMessage(fsPath);
    }
}

void DLLLoadDispatcher::DispatchModuleLoadMessage(const std::filesystem::path& modulePath) {
    if (modulePath.filename() == L"client.dll") {
        this->ISys().PublishSync("Hook/LoadLibraryExW/client.dll"_hash);
    }
    if (modulePath.filename() == L"engine2.dll") {
        this->ISys().PublishSync("Hook/LoadLibraryExW/engine2.dll"_hash);
    }
    if (modulePath.filename() == L"tier0.dll") {
        this->ISys().PublishSync("Hook/LoadLibraryExW/tier0.dll"_hash);
    }
    if (modulePath.filename() == L"panorama.dll") {
        this->ISys().PublishSync("Hook/LoadLibraryExW/panorama.dll"_hash);
    }
}