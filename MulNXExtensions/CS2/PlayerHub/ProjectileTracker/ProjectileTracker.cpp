#include "ProjectileTracker.hpp"

#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

static std::string GetControllerPlayerName(CS2::CCSPlayerController* pController) {
    if (!pController) return "未知玩家";
    auto namePtr = pController->m_iszPlayerName();
    if (!namePtr) return "未知玩家";
    auto name = MulNX::Memory::ReadString(namePtr);
    return name.empty() ? "未知玩家" : name;
}

void ProjectileTracker::Menu(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("投掷物追踪器", this->showWindow);
    if (!w) return;
    MulNX::UI::Checkbox("启用功能", this->Enable);
    try {
        std::shared_lock lock(this->smutex);

        
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在绘制投掷物追踪窗口时发生异常：{}", e.what()));
    }
}

bool ProjectileTracker::Init() {
    this->ISys().SendTask("Main", "CSControl", [this]() {
        this->Main();
        return true;
        });

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        this->Menu(node);
        });

    this->ISys()
        .SubscribeAsync("Game/Entity/Added")
        .SubscribeAsync("Game/Entity/Removed");

    return true;
}

void ProjectileTracker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Game/Entity/Added"_hash: {
        auto pEntity = msg.p1.as<CS2::C_BaseEntity*>();
        auto hEntity = msg.p2.low<CS2::CHandleBase>();
        std::string name;
        try {
            name = pEntity->GetName();
        }
        catch (const std::exception& e) {
            this->ISys().LogWarning("添加：在分析实体消息以分流时发生异常");
        }
        if (name.find("projectile") != std::string::npos) {
            this->HandleProjectileAdd(pEntity->As<CS2::C_BaseCSGrenadeProjectile>(), std::move(name));
        }
        // else if (name.find("grenade") != std::string::npos) {
        //     this->HandleGrenadeAdd(pEntity->As<CS2::C_BaseCSGrenade>(), std::move(name));
        // }
        break;
    }
    case "Game/Entity/Removed"_hash: {
        auto pEntity = msg.p1.as<CS2::C_BaseEntity*>();
        break;
    }
    }
}

void ProjectileTracker::HandleProjectileAdd(CS2::C_BaseCSGrenadeProjectile* pProjectile, std::string&& name) {
    try {
        auto hThrower = MulNX::MRead(pProjectile->m_hThrower());
        auto* pPawn = this->CS2->client.GetBaseEntityFromHandle(hThrower)->As<CS2::C_CSPlayerPawn>();
        auto hController = MulNX::MRead(pPawn->m_hController());
        auto* pController = this->CS2->client.GetBaseEntityFromHandle(hController)->As<CS2::CCSPlayerController>();
        if (!pController)return;
        std::unique_lock lock(this->smutex);
        this->ISys().LogInfo(std::format("记录 projectile({}) -> 控制器 SteamID={} ", name, MulNX::MRead(pController->m_steamID())));

        auto* pLocalPlayerPawn = this->CS2->client.GetLocalPlayerPawn();
        if (!pLocalPlayerPawn)return;
        auto* pObserverServices = MulNX::MRead(pLocalPlayerPawn->pObserverServices());
        if (!pObserverServices)return;
        auto hTargetObserverPawn = MulNX::MRead(pObserverServices->hObserverTarget());
        auto* pTargetPawn = this->CS2->client.GetBaseEntityFromHandle(hTargetObserverPawn)->As<CS2::C_CSPlayerPawn>();
        if (!pTargetPawn)return;
        auto hTargetController = MulNX::MRead(pTargetPawn->m_hController());
        auto* pTargetController = this->CS2->client.GetBaseEntityFromHandle(hTargetController);

        if (pController == pTargetController) {
            this->pTargetWatchProjectile.store(pProjectile, std::memory_order_release);
        }

        return;
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在分析新增实体时发生异常：{}", e.what()));
    }
}

void ProjectileTracker::Main() {
    this->Update();
    if (!this->Enable.load(std::memory_order_acquire))return;
    try {
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
        this->ISys().LogWarning(std::format("在追踪投掷物时发生异常：{}", e.what()));
    }
}

std::optional<MulNX::Math::View> ProjectileTracker::GetView() {
    if (!this->Enable.load(std::memory_order_acquire))return std::nullopt;
    if (!this->pTargetWatchProjectile.load(std::memory_order_acquire)) return std::nullopt;
    return *this->currentView.Read();
}