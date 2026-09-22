#pragma once
#include <CameraSystem/CamMacro/CamMacro.hpp>
#include <CameraSystem/CameraSystem.hpp>

class CamMacroManager final :public CamSysModule {
    MulNX::IPCer* pIPCer = nullptr;
    std::unordered_map<std::string, std::unique_ptr<CamMacro>> camMacros{};
    CamMacro* pOperatingMacro = nullptr;

    bool shortcutEnable = true;

    mutable std::atomic<MulNX::KeyCheckPack> bufKCPack{};
    mutable std::atomic<bool> bKCPWindow = false;

    void UI()const;
    void CamMacroShowOneLine(const CamMacro* pCamMacro)const;
    void CamMacroDebugWindow(const CamMacro* pMacro)const;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    CamMacro* FindCamMacro(const std::string& name);

    bool CamMacroCreate(const std::string& name);
    bool CamMacroSaveAll();
    bool CamMacroLoad(const std::filesystem::path& pathMacro);
    bool CamMacroDelete(const std::string& name);
    bool CamMacroClearAll();

    void PlayCamMacro(const std::string& name);
public:
    bool Menu();
    void HandleUpdate();
};