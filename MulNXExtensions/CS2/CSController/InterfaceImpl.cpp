#include "CSController.hpp"

float* CSController::GetViewMatrix() {
    return this->client.dwViewMatrix();
}
MulNX::Math::View CSController::GetView() {
    MulNX::Math::View view;
    {
        auto read = this->controlView.currentView.Read();
        view.position = read->position;
        view.rotation = read->rotation;
        view.FOV = read->FOV;
    }

    view.dof.NearBlurry = *this->controlView.dofs.pNearBlurry;
    view.dof.NearCrisp = *this->controlView.dofs.pNearCrisp;
    view.dof.FarCrisp = *this->controlView.dofs.pFarCrisp;
    view.dof.FarBlurry = *this->controlView.dofs.pFarBlurry;

    return view;
}
float CSController::GetTime() {
    try {
        float time = MulNX::MRead(this->CSGlobalVars->fCurrentTime());
        // float timereal = MulNX::MRead(this->CSGlobalVars->fRealTime());
        // auto iTime2 = MulNX::MRead(this->CSGlobalVars->iTickCount());
        // auto fTime2 = static_cast<float>(iTime2) / 64.0f;
        // 经过验证，fCurrentTime更稳定一点
        return time;
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogError("读取游戏时间失败");
        return 0;
    }
    
}
bool CSController::JumpTime(const float time) {
    int currentGameTick = this->Time()->GetReal() * 64;
    int currentDemoTick = this->GetDemoTick();

    int targetGameTick = static_cast<int>(time * 64);
    int deltaTick = currentGameTick - currentDemoTick;
    int tick = targetGameTick - deltaTick;

    std::string command = std::format("demo_gototick {}", tick);
    this->ISys().AsyncCommand(std::move(command));
    return true;
}
float CSController::GetWinWidth()const {
    return this->controlView.WindowWidth.load(std::memory_order_relaxed);
}
float CSController::GetWinHeight()const {
    return this->controlView.WindowHeight.load(std::memory_order_relaxed);
}
bool CSController::SpecPlayer(int IndexInMap) {
    this->ISys().AsyncCommand("spec_mode 2;spec_player " + std::to_string(this->AL3DGameData.Players[IndexInMap].IndexInMap));
    return true;
}
D_Player& CSController::GetPlayerMsg(int Index) {
    //std::shared_lock lock(this->GetMtx());
    return this->AL3DGameData.Players[Index];
}
void CSController::spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot) {
    this->ISys().AsyncCommand(std::format("spec_goto {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y));
    this->controlView.InputRoll.store(rot.z, std::memory_order_release);
}
void CSController::ClearViewOverride() {
    this->controlView.hasViewToGame.store(false, std::memory_order_release);
}
void CSController::SetDOF(const MulNX::Math::DOFParam& dof) {
    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;
}