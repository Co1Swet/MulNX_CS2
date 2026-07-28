#include "ProjectileTracker.hpp"
#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNX/Base/UI/UI.hpp>

static std::string GetControllerPlayerName(CS2::CCSPlayerController* pController) {
    if (!pController) return "未知玩家";
    auto namePtr = pController->m_iszPlayerName();
    if (!namePtr) return "未知玩家";
    auto name = MulNX::Memory::ReadString(namePtr).value_or("读取失败");
    return name.empty() ? "未知玩家" : name;
}

void ProjectileTracker::Menu() {
    MulNX::UI::Checkbox("启用投掷物追踪", this->enable);
}

bool ProjectileTracker::Init() {
    this->SendTask("Main", "CSControl", [this]() {
        this->Main();
        return true;
        });

    (*this)
        .SubscribeSync("Hook/AddEntity", [this](MulNX::Message& msg) {return this->OnEntityAdd(msg);})
        .SubscribeSync("Hook/RemoveEntity", [this](MulNX::Message& msg) {return this->OnEntityRemove(msg);})
        ;

    this->UIRegisterCallback("UI.View", [this](auto&&...) {return this->Menu();});

    return true;
}

void ProjectileTracker::OnEntityAdd(MulNX::Message& msg) {
    auto&& [pEntity] = msg.Access<CS2::C_BaseEntity*>();
    std::string name;
    try {
        name = pEntity->GetName();
    }
    catch (const std::exception& e) {
        this->LogWarning("添加：在分析实体消息以分流时发生异常");
    }
    if (name.find("projectile") != std::string::npos) {
        std::unique_lock lock(this->smutex);
        this->bufferProjectiles.insert(pEntity->As<CS2::C_BaseCSGrenadeProjectile>());
    }
}

void ProjectileTracker::OnEntityRemove(MulNX::Message& msg) {
    auto&& [pEntity] = msg.Access<CS2::C_BaseEntity*>();
    std::string name;
    try {
        name = pEntity->GetName();
    }
    catch (const std::exception& e) {
        this->LogWarning("添加：在分析实体消息以分流时发生异常");
    }
    if (name.find("projectile") != std::string::npos) {
        std::unique_lock lock(this->smutex);
        this->bufferProjectiles.erase(pEntity->As<CS2::C_BaseCSGrenadeProjectile>());
        if (this->pTargetWatchProjectile.load() == pEntity) {
            this->pTargetWatchProjectile.store(nullptr);
        }
    }
    
}

bool ProjectileTracker::HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile) {
    try {
        auto hThrower = MulNX::MRead(pProjectile->m_hThrower());
        auto* pPawn = this->CS2->client.GetBaseEntityFromHandle(hThrower)->As<CS2::C_CSPlayerPawn>();
        auto hController = MulNX::MRead(pPawn->m_hController());
        auto* pController = this->CS2->client.GetBaseEntityFromHandle(hController)->As<CS2::CCSPlayerController>();
        if (!pController)return false;

        this->LogInfo(std::format("记录 projectile({}) -> 控制器 SteamID={} ", pProjectile->GetName(), MulNX::MRead(pController->m_steamID())));

        auto* pObPawn = this->CS2->client.TryGetObservingPawn();
        if(!pObPawn) return true;
        auto hTargetController = MulNX::MRead(pObPawn->m_hController());
        auto* pTargetController = this->CS2->client.GetBaseEntityFromHandle(hTargetController);

        if (pController == pTargetController) {
            this->pTargetWatchProjectile.store(pProjectile, std::memory_order_release);
        }

        return true;
    }
    catch (const std::exception& e) {
        this->LogWarning(std::format("在分析新增实体时发生异常：{}", e.what()));
        return false;// 可能是因为实体数据尚未完全初始化，继续尝试直到成功或确认不相关
    }
}

void ProjectileTracker::Main() {
    this->Update();
    if (!this->enable.load(std::memory_order_acquire))return;
    std::unique_lock lock(this->smutex);
    try {
        for (auto it=this->bufferProjectiles.begin(); it != this->bufferProjectiles.end();) {
            if (this->HandleProjectileAdd(*it)) {
                it = this->bufferProjectiles.erase(it);
            }
            else {
                ++it;
            }
        }

        CS2::C_BaseCSGrenadeProjectile* pProjectile = this->pTargetWatchProjectile.load(std::memory_order_acquire);
        if (!pProjectile) return;

        auto pGameSceneNode = MulNX::MRead(pProjectile->pGameSceneNode());
        if (!pGameSceneNode) return;

        int32_t nBounces = MulNX::MRead(pProjectile->m_nBounces());

        auto pos = MulNX::MRead(pGameSceneNode->vecOrigin());
        auto vel = MulNX::MRead(pProjectile->m_vecVelocity());

        // 相机后移距离（可根据投掷物尺寸调整）
        constexpr float kCameraOffsetDistance = 80.0f;

        DirectX::XMFLOAT3 viewRot{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 cameraPos = pos; // 默认相机位置为投掷物位置

        // 检查速度是否有效（非零向量）
        float speedSq = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;
        const float kEpsilon = 1e-6f;
        if (speedSq > kEpsilon) {
            // 归一化速度方向
            float invLen = 1.0f / std::sqrt(speedSq);
            DirectX::XMFLOAT3 dir{ vel.x * invLen, vel.y * invLen, vel.z * invLen };

            // 将方向向量转换为欧拉角（pitch, yaw, roll）
            MulNX::Math::CSDirToEuler(dir, viewRot);

            // 后移相机：位置 = 投掷物位置 - 速度方向 * 距离
            cameraPos.x = pos.x - dir.x * kCameraOffsetDistance;
            cameraPos.y = pos.y - dir.y * kCameraOffsetDistance;
            cameraPos.z = pos.z - dir.z * kCameraOffsetDistance;
        }
        // 若速度为零，保持相机位置与投掷物重合，角度保持不变（可扩展为平滑过渡）
        {
            auto write = this->currentView.Write();
            write->position = cameraPos;
            write->rotation = viewRot;
        }
    }
    catch (const std::exception& e) {
        this->LogWarning(std::format("在追踪投掷物时发生异常：{}", e.what()));
    }
}

bool ProjectileTracker::HandleUpdate(CS2::CViewSetup* viewSetup, const int& num) {
    if (!this->enable.load(std::memory_order_acquire))return false;
    if (!this->pTargetWatchProjectile.load(std::memory_order_acquire)) return false;
    auto view = this->currentView.Read();
    *viewSetup->pViewOrigin() = view->position;
    *viewSetup->pViewAngles() = view->rotation;
    return true;
}