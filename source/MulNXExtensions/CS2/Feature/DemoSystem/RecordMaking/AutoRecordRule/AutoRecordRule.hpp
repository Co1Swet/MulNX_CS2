#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class AutoRecordRule final : public DemModuleBase {
public:
    enum class Mode {
        SinglePlayer,
        AllPlayers
    };
    struct Options {
        Mode mode = Mode::AllPlayers;
        Steam64UID player = 0;
        bool includeVictimView = false;
        bool autoStart = false;
    };
    struct Plan {
        std::vector<MulNX::Message> requests;
        bool autoStart = false;
    };
private:
    Options options{};

    void Window();
    bool Init() override;
public:
    Options GetOptions() const;
    void SetOptions(const Options& newOptions);
    Plan BuildPlan(const Demo::Info& info) const;
};
