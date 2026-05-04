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
    this->ISys().AsyncCommand("spec_mode 2;spec_player " + std::to_string(this->CS2EBGameData.Players[IndexInMap].IndexInMap));
    return true;
}
D_Player& CSController::GetPlayerMsg(int Index) {
    //std::shared_lock lock(this->GetMtx());
    return this->CS2EBGameData.Players[Index];
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

MulNX::TimeBridge::TimeBridge(CSController* pCS2) : pCS2(pCS2) {
    this->startTime = std::chrono::steady_clock::now();
}

void MulNX::TimeBridge::update() {
    float time = this->pCS2->GetTime();
    if (time > this->lastRealTime) {
        this->lastRealTime = time;
    }
    else if (this->lastRealTime - time > 0.025f) {
        this->lastRealTime = time;
    }
    return;
}

bool MulNX::TimeBridge::RefreshVirtual(bool virtualTimePlaying, float scale) {
    this->update();
    this->refreshTime = this->lastRealTime;
    this->startTime = std::chrono::steady_clock::now();
    this->scale = scale;
    this->virtualTimePlaying = virtualTimePlaying;
    return true;
}

float MulNX::TimeBridge::GetReal() {
    this->update();
    return this->lastRealTime;
}

bool MulNX::TimeBridge::JumpReal(float time) {
    return this->pCS2->JumpTime(time);
}

bool MulNX::TimeBridge::JumpRealRel(float time) {
    return this->JumpReal(time + this->GetReal());
}

float MulNX::TimeBridge::GetVirtual() {
    // 这里不需要更新，因为虚拟时间的更新是由RefreshVirtual控制的，GetVirtual只负责计算当前的虚拟时间
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - this->startTime).count();
    return this->refreshTime + elapsed * this->scale;
}

float MulNX::TimeBridge::Get() {
    return this->virtualTimePlaying ? this->GetVirtual() : this->GetReal();
}


MulNX::TimeBridge* CSController::Time() {
    return &this->timeBridge;
}