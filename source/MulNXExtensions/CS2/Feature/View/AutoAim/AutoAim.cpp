#include "AutoAim.hpp"

bool AutoAim::Init() {

    return true;
}

bool AutoAim::HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) {
    if (!this->enable.load(std::memory_order_acquire)) return false;
    try {
        DirectX::XMFLOAT3 currentCamPos = *viewSetup->pViewOrigin();
        auto pAngle = viewSetup->pViewAngles();                 // 渲染视角指针
        auto localAngle = this->CS2->client.dwViewAngles();     // 引擎视角指针（用户确认是指针）
        if (!pAngle) return false;

        // 定义一个 lambda，用于同时修改两个视角角度
        auto setAngles = [&](const DirectX::XMFLOAT3& angles) {
            if (pAngle) {
                pAngle->x = angles.x;
                pAngle->y = angles.y;
                pAngle->z = angles.z;
            }
            if (localAngle) {
                localAngle[0] = angles.x;
                localAngle[1] = angles.y;
                localAngle[2] = angles.z;
            }
            };

        float bestDistSq = FLT_MAX;
        DirectX::XMFLOAT3 bestTargetPos;

        // 遍历实体，选择距离摄像机最近的敌人
        for (int i = 0; i < 32; ++i) {
            auto pCtrl = this->CS2Entitys->GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!pCtrl || this->CS2->client.dwLocalPlayerController() == pCtrl) continue;

            auto hPawn = MulNX::MRead(pCtrl->m_hPlayerPawn());
            if (!hPawn.Valid()) continue;
            auto pPawn = this->CS2Entitys->GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
            if (!pPawn) continue;
            if (MulNX::MRead(pPawn->iHealth()) == 0)continue;

            auto eyePos = pPawn->GetEyePos();

            float dx = eyePos.x - currentCamPos.x;
            float dy = eyePos.y - currentCamPos.y;
            float dz = eyePos.z - currentCamPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTargetPos = eyePos;
            }
        }

        if (bestDistSq < FLT_MAX) {
            // 计算指向最近目标的方向
            DirectX::XMFLOAT3 dirToBest = {
                bestTargetPos.x - currentCamPos.x,
                bestTargetPos.y - currentCamPos.y,
                bestTargetPos.z - currentCamPos.z
            };
            DirectX::XMFLOAT3 newAngles;
            MulNX::Math::CSDirToEuler(dirToBest, newAngles);
            newAngles.z = 0.0f;   // 保持 roll = 0

            // 使用 lambda 同时修改两个视角
            setAngles(newAngles);

            return true;
        }
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
    }

    return false;
}