#pragma once
#include <MulNX/MulNX.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <CameraSystem/CamPlayScheduler/CamPlayRequest.hpp>

class WrapCampath {
public:
    std::string campathName;
    float offset = 0;
};

class CamMacro final {
    std::string name{};
    std::vector<WrapCampath> wrapCampaths;

    PlaybackMode playmode = PlaybackMode::Orchestration;
    std::atomic<MulNX::KeyCheckPack> KCPack{};
    mutable bool dirty = false;
public:
    CamMacro(const std::string& name) :
        name(name) {
        this->dirty = true;
    }

    bool AddCampath(const std::string& name, const float offset);
    bool RemoveCampath(const std::string& campathName);
    inline const std::vector<WrapCampath>& GetVec()const { return this->wrapCampaths; }

    std::pair<bool, std::string> Save(const std::filesystem::path& folderPath)const;
    std::pair<bool, std::string> Load(YAML::Node& root);

    void Clear();
    void ResetName(std::string_view NewName);
    const std::string& GetName()const;
    std::string GetMsg();

    inline const std::atomic<MulNX::KeyCheckPack>& GetKeyCheckPack()const { return this->KCPack; }
    void SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack);
};