#pragma once
#include <MulNXExtensions/TimeLiner/ITimeRend.hpp>
#include <Feature/DemoSystem/DemBase/DemModuleBase.hpp>

class DemoEventsRender final : public CSModuleBase, public ITimeRend {
    std::map<std::string, Demo::Info> m_demos;
    std::string m_currentDemoName;

    void TimeRend(TimeLiner* timeline, ImDrawList* dl) override;
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;  // 新增
};