#include "WorldDraw.hpp"
#include <Intro/HookView/HookView.hpp>

bool WorldDraw::Init() {
    this->pParticleManager = this->FindModule<ParticleManager>("ParticleManager");
    this->pParticleSystemMgr = this->FindModule<ParticleSystemMgr>("ParticleSystemMgr");

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) {
        this->OnFirst();
        try {
            this->UpdateMyDraw();
        }
        catch (...) {
            this->LogError("错误");
        }
        
        });

    return true;
}

void WorldDraw::OnFirst() {
    if (!this->pInputSystem->CheckComboClick('J', 2))return;

    int idx = -1;

    int result = this->pParticleManager->CreateParticle(&idx,
        "particles/entity/spectator_utility_trail.vpcf",
        8, 0, 0, 0, 0);
    if (result == -1 || idx == -1) {
        MulNX::ErrorTerminate("粒子创建失败！");
    }
    this->LogSucc("粒子创建成功");

    float color[3] = { 1.0f, 0.0f, 0.0f };
    this->pParticleManager->UpdateParticle(idx, 0x10, color, 0);   // 红色
    float props[3] = { 4.0f, 1.0f, 1.0f };         // lifetime, width, alpha
    this->pParticleManager->UpdateParticle(idx, 0x3, props, 0);

    int64_t handle = 0;
    void* nullName = nullptr; // 空字符串
    if (this->pParticleSystemMgr->CreateTrailHandle(&handle, &nullName) == 0 || handle == 0) {
        MulNX::ErrorTerminate("未得到有效句柄");
    }
    this->LogSucc("句柄创建成功");

    if (!this->pParticleManager->BindTrail(idx, 0, handle)) {
        MulNX::ErrorTerminate("绑定失败！");
    }

    this->myTrail.particleIndex = idx;
    this->myTrail.trailHandle = handle;

    // 记录基准时间
    this->m_flTrailBaseTime = *this->CS2->CSGlobalVars->fCurrentTime()-3;
    this->m_flLastSampleTime = this->m_flTrailBaseTime;

    this->LogSucc("绑定成功");
}

void WorldDraw::UpdateMyDraw() {
    if (this->pInputSystem->CheckComboClick('K', 2)) {
        this->myTrail = ParticleTrail{};
        this->first = true;
    }

    if (this->myTrail.particleIndex == -1 || this->myTrail.trailHandle == 0)
        return;

    float curTime = *this->CS2->CSGlobalVars->fCurrentTime()-3;

    // 时间片控制：每隔 m_flSampleInterval 采样一次
    if (curTime - this->m_flLastSampleTime < this->m_flSampleInterval)
        return;
    this->m_flLastSampleTime = curTime;

    // 获取新位置
    Vector3 newPos;
    auto camPos = this->CS2View->GetView().position;

    auto pOBing = this->CS2Entitys->TryGetObservingPawn();
    if (!pOBing)return;
    auto hActiveWeapon = pOBing->GetHandleActiveWeapon();
    auto* pWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hActiveWeapon)->As<CS2::C_BasePlayerWeapon>();
    if (!pWeapon) return;
    camPos = pWeapon->GetBonePos(1);

    newPos.x = camPos.x;
    newPos.y = camPos.y;
    newPos.z = camPos.z;

    // 计算相对时间
    float relativeTime = curTime - this->m_flTrailBaseTime;

    // 添加新点
    if (this->myTrail.positions.size() < this->myTrail.MAX_POINTS) {
        this->myTrail.positions.push_back(newPos);
        this->myTrail.times.push_back(relativeTime);
    }
    else {
        // 已满，整体左移一个位置，覆盖最旧的元素（内存地址不变）
        std::move(this->myTrail.positions.begin() + 1, this->myTrail.positions.end(), this->myTrail.positions.begin());
        std::move(this->myTrail.times.begin() + 1, this->myTrail.times.end(), this->myTrail.times.begin());
        this->myTrail.positions.back() = newPos;
        this->myTrail.times.back() = relativeTime;
    }

    // 准备控制点数据
    void* controlPointData[24] = { nullptr };
    controlPointData[0] = this->myTrail.positions.data(); // 位置数组
    controlPointData[17] = this->myTrail.times.data();    // 时间数组

    // 调用更新接口
    this->pParticleSystemMgr->UpdateTrail(this->myTrail.trailHandle,
        (int)this->myTrail.positions.size(),
        controlPointData);
}