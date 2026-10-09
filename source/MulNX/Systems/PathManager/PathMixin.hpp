#pragma once
#include <MulNX/Core/Module/IModule.hpp>
#include <MulNXThirdParty/ghc/fs.hpp>

namespace MulNX {
    class PathManager;
    template<typename T>
    class PathMixin {
        T* This() { return static_cast<T*>(this); }
        PathManager* pPath = nullptr;
    public:
        PathMixin() {
            This()->preInits.push_back([this]() {
                this->pPath = This()->FindModule<PathManager>("PathManager");
                return true;
                });
        }
        fs::path PathGet(const std::string& Target) {
            return this->pPath->PathGetForModule(This()->GetName(), Target);
        }
        PathManager* Path() {
            return this->pPath;
        }
    };
}