#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/CamPack/CamPack.hpp>

class CamPackManager final :public CamSysModule {
    MulNX::IPCer* pIPCer = nullptr;
    std::unordered_map<std::string, std::shared_ptr<CamPack>> camPacks{};

    mutable std::atomic<bool> shortcutEnable = true;
    mutable std::atomic<bool> bKCPWindow = false;
    mutable std::atomic<MulNX::KeyCheckPack> bufKCPack{};

    std::shared_ptr<CamPack> pOperatingCamPack = nullptr;

    void CamPackDebugWindow()const;
    void UI()const;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    std::shared_ptr<CamPack> FindCamPack(const std::string& name);

    bool CamPackDelete(const std::string& name);
    bool CamPackClearAll();
    bool CamPackCreate(const std::string& name);
    bool CamPackSave();
    bool CamPackRefresh();
    bool CamPackLoad(const std::filesystem::path& projectPath, const std::string& name);
    bool CamPackApply(const std::shared_ptr<CamPack> pCamPack);
public:
    std::shared_ptr<CamPack> pActiveCamPack = nullptr;
    void Menu()const;
    void HandleUpdate();
};