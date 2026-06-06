#include "PathManager.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/IPCer/IPCer.hpp>
#include <MulNXThirdParty/All_pugixml.hpp>

bool MulNX::PathManager::Init() {
    this->IPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");
    this->Root = this->IPCer->GetRoot();
    this->CoreName = this->Core->GetName();
    this->CoreRoot = this->Root / this->CoreName;
    this->LoadPathLists(this->Root / "PathLists.xml");
    this->CheckShared();
    return true;
}

bool MulNX::PathManager::LoadPathLists(const std::filesystem::path& xmlPath) {
    pugi::xml_document xml;
    pugi::xml_parse_result result = xml.load_file(xmlPath.c_str());
    if (!result)MulNX::ErrorTerminate("PathLists.xml加载失败，请检查");
    auto nodePath = xml.child("Path");
    auto nodeShared = nodePath.child("Shared");
    if (!nodeShared)MulNX::ErrorTerminate("PathLists.xml文件疑似损坏");
    for (const auto& nodeShardPath : nodeShared.children("Path")) {
        // 这里不需要担心损坏，最差是空
        this->Shareds.push_back(nodeShardPath.attribute("Name").as_string());
    }
    return true;
}
bool MulNX::PathManager::CheckShared() {
    for (const auto& shared : this->Shareds) {
        std::filesystem::path Path = this->Root / shared;
        if (std::filesystem::exists(Path)) {
            this->ISys().LogInfo("检测到共享目录已创建：" + Path.string());
        }
        else {
            try {
                std::filesystem::create_directory(Path);
                this->ISys().LogSucc("成功创建新的共享目录" + Path.string());
            }
            catch (const std::exception& e) {
                this->ISys().LogError("在创建共享目录" + Path.string() + "时出现错误：" + e.what());
            }
        }
    }
    return true;
}

std::filesystem::path MulNX::PathManager::PathGetForModule(const std::string& ModuleName, const std::string& Target) {
    auto path = this->CoreRoot / ModuleName / Target;
    if (!std::filesystem::exists(path)) {
        this->ISys().LogInfo("模块[" + ModuleName + "]尝试访问不存在的文件夹，将尝试为其创建");
        try {
            // 可创建多级目录
            if (!std::filesystem::create_directories(path)) {
                this->ISys().LogError("创建文件夹失败！路径：" + path.string());
                return {};
            }
            else {
                this->ISys().LogSucc("文件夹创建成功：" + path.string());
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            MulNX::ErrorTerminate("创建文件夹时发生文件系统错误：" + std::string(e.what()) + " 路径：" + path.string());
        }
        catch (...) {
            MulNX::ErrorTerminate("在创建文件夹时发生未知错误：" + path.string());
        }
    }
    else if (!std::filesystem::is_directory(path)) {
        MulNX::ErrorTerminate("模块[" + ModuleName + "]的路径存在但不是文件夹：" + path.string());
    }

    return path;
}

std::filesystem::path MulNX::PathManager::PathGetForShared(const std::string& Target) {
    auto path = this->Root / Target;

    if (!std::filesystem::exists(path)) {
        this->ISys().LogInfo("共享资源路径不存在，将尝试为其创建：" + path.string());
        try {
            // 可创建多级目录
            if (!std::filesystem::create_directories(path)) {
                this->ISys().LogError("创建共享目录失败！路径：" + path.string());
                return {};
            }
            else {
                this->ISys().LogSucc("共享目录创建成功：" + path.string());
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            MulNX::ErrorTerminate("创建共享目录时发生文件系统错误：" + std::string(e.what()) + " 路径：" + path.string());
        }
        catch (...) {
            MulNX::ErrorTerminate("在创建共享目录时发生未知错误：" + path.string());
        }
    }
    else if (!std::filesystem::is_directory(path)) {
        MulNX::ErrorTerminate("共享路径存在但不是文件夹：" + path.string());
    }

    return path;
}