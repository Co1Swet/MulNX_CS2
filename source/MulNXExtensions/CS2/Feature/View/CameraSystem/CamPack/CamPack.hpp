#pragma once
#include <MulNX/MulNX.hpp>
#include <filesystem>
#include <atomic>

class CamPack {
    std::string name{};
    std::string description{};
    std::atomic<MulNX::KeyCheckPack> KCPack{};

public:
    std::vector<std::string> OnNewRound{};
    std::vector<std::string> OnRoundStart{};
    std::vector<std::string> OnRoundEnd{};

    explicit CamPack(const std::string& name) : name(name) {}

    inline const std::string& GetName() const { return this->name; }
    inline const std::string& GetDescription() const { return this->description; }

    inline const std::atomic<MulNX::KeyCheckPack>& GetKeyCheckPack() const { return this->KCPack; }
    void SetKeyCheckPack(const MulNX::KeyCheckPack& kcp) {
        this->KCPack.store(kcp, std::memory_order_release);
    }

    void ResetName(const std::string& newName);
    void Refresh();

    std::pair<bool, std::string> Save(const std::filesystem::path& dir);
};