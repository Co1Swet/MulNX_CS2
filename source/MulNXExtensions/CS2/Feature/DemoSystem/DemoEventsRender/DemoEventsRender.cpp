#include "DemoEventsRender.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool DemoEventsRender::Init() {
    // 注册到时间轴
    this->FindModule<TimeLiner>("TimeLiner")->timeLineModules.push_back(this);

    // 订阅所需消息
    this->SubscribeAsync("Observe/SpecSteam64UID");
    this->SubscribeAsync("Demo/InfoLoad");
    this->SubscribeAsync("Demo/SetOperating");

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemoEventsRender::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/InfoLoad"_hash: {
        auto info = std::move(*msg.asp.get<Demo::Info>());
        m_demos[info.demoFileName] = std::move(info);
        break;
    }
    case "Demo/SetOperating"_hash: {
        m_currentDemoName = msg.asp.get<MulNX::NetExt>()->str1;
        break;
    }
    }
}

void DemoEventsRender::TimeLineCallback(TimeLiner* timeline, ImDrawList* dl) {
    if (m_currentDemoName.empty())
        return;

    auto itDemo = m_demos.find(m_currentDemoName);
    if (itDemo == m_demos.end())
        return;

    const auto& info = itDemo->second;

    auto oSteam64uid = this->CS2->client.TryGetObservingSteam64UID();
    if (!oSteam64uid)return;
    auto Steam64uid = *oSteam64uid;

    auto itPlayer = info.players.find(Steam64uid);
    if (itPlayer == info.players.end())
        return;

    const auto& player = itPlayer->second;
    if (player.roundInfo.empty())
        return;

    // 获取当前时间轴范围（用于过滤，但 Map 本身会裁剪）
    float minTime = timeline->minTime;
    float maxTime = timeline->maxTime;
    if (maxTime <= minTime)
        return;

    // 辅助：将 tick 转为秒
    auto tickToSec = [](int tick) { return tick / 64.0f; };

    // 绘制事件的辅助 lambda
    auto drawEvent = [&](const Demo::KillEvent& ev, bool isKill) {
        float time = tickToSec(ev.tick);
        if (time < minTime || time > maxTime)
            return;

        // 使用层偏移：根据事件数量动态分配层，这里简单固定层0
        int layer = 0;
        ImVec2 pos = timeline->Map(time, layer);

        // 颜色：击杀红色，被击杀蓝色
        ImU32 color = isKill ? IM_COL32(255, 50, 50, 255) : IM_COL32(50, 50, 255, 255);
        float radius = 5.0f;

        // 绘制光晕
        dl->AddCircleFilled(pos, radius + 2.0f, IM_COL32(255, 255, 255, 80));
        // 主圆点
        dl->AddCircleFilled(pos, radius, color);

        // 可选：绘制外圈
        dl->AddCircle(pos, radius, IM_COL32(255, 255, 255, 150), 0, 1.5f);

        // 存储标记位置用于悬停检测（可简化：使用 ImGui 区域检测）
        // 这里利用 ImGui 的 ID 堆栈，但为了简单，直接检测鼠标位置并显示 tooltip
        ImVec2 mouse = ImGui::GetMousePos();
        float dist = sqrtf((mouse.x - pos.x) * (mouse.x - pos.x) + (mouse.y - pos.y) * (mouse.y - pos.y));
        if (dist < 10.0f && ImGui::IsMouseHoveringRect(ImVec2(pos.x - 10, pos.y - 10), ImVec2(pos.x + 10, pos.y + 10))) {
            ImGui::BeginTooltip();
            std::string desc;
            if (isKill) {
                desc = std::format("击杀 {} ({}，回合 {})",
                    info.GetPlayerName(ev.victimSteamId),
                    ev.weaponName,
                    ev.roundNumber);
            }
            else {
                desc = std::format("被 {} 击杀 ({}，回合 {})",
                    info.GetPlayerName(ev.killerSteamId),
                    ev.weaponName,
                    ev.roundNumber);
            }
            ImGui::Text("%s", desc.c_str());
            ImGui::EndTooltip();
        }
        };

    // 遍历所有回合的事件
    for (const auto& [round, roundInfo] : player.roundInfo) {
        // 绘制击杀事件
        for (const auto& ev : roundInfo.killEvents) {
            drawEvent(ev, true);
        }
        // 绘制被击杀事件
        if (roundInfo.Bekilled.has_value()) {
            drawEvent(roundInfo.Bekilled.value(), false);
        }
    }
}