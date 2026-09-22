#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <MulNXUtils/NewestBuffer.hpp>

class CampathManager final : public CamSysModule {
    CameraDrawer* CamDrawer = nullptr;
    MulNX::IPCer* pIPCer = nullptr;

    std::unordered_map<std::string, std::shared_ptr<FreeCameraPath>> campaths;
    std::atomic<std::shared_ptr<FreeCameraPath>> pOperatingCampath = nullptr;

    void CampathShowOneLine(const FreeCameraPath* pCampath)const;
    void DebugUI(const FreeCameraPath* campath)const;
    void UI()const;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    FreeCameraPath* CampathCreate(const std::string& name);
    bool CampathSaveAll();
    bool CampathLoad(const std::filesystem::path& pathCampath);
    // 返回true表示名称现在可用
    bool CampathDelete(const std::string& name);
    bool CampathClearAll();
public:
    std::shared_ptr<FreeCameraPath> FindCampath(const std::string& name,
        std::source_location where = std::source_location::current());
    void Menu()const;
    void HandleUpdate();
};