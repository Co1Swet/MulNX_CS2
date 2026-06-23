#include "TrailsController.hpp"
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/ParticleManager/ParticleManager.hpp>

using DrawStuff_t = void(*)(CS2::C_BaseCSGrenadeProjectile*, char);

bool TrailsController::Init() {
    this->pParticleMgr = this->FindModule<ParticleManager>("ParticleManager");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Projectile::Func_BaseCSGrenadeProjectile_DrawStuff).Data();
        this->hkFunc_BaseCSGrenadeProjectile_DrawStuff = this->CreateHook("C_BaseCSGrenadeProjectile_DrawStuff", target, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pProjectile = (CS2::C_BaseCSGrenadeProjectile*)ctx->rcx;
            char flag = *(char*)&ctx->rdx;
            auto pOriginalDrawStuff = (DrawStuff_t)hk->pMaybeRawFunc;
            float curtime = MulNX::MRead(this->CS2->CSGlobalVars->fCurrentTime());
            int* pEffectIndex = pProjectile->m_nSnapshotTrajectoryEffectIndex();
            // 未创建
            if (*pEffectIndex == -1) {
                // 写入轨迹创建时间
                *pProjectile->m_flTrajectoryTrailEffectCreationTime() = curtime;
                // 创建粒子
                int newIdx = -1;
                this->pParticleMgr->CreateParticle(&newIdx, "particles/entity/spectator_utility_trail.vpcf", 8, 0, 0, 0, 0);
                if (newIdx != -1) {
                    // 保存粒子索引
                    *pEffectIndex = newIdx;
                    // 设置颜色（固定绿色，RGB 0, 255, 0）
                    struct { float r, g, b; } colorData = { 0.0f, 255.0f, 0.0f };
                    this->pParticleMgr->UpdateParticle(newIdx, 0x10, &colorData, 0);
                    // 调用原函数，让粒子从正确的位置开始绘制
                    pOriginalDrawStuff(pProjectile, flag);
                }
                return MulNX::Hook::Then::Return;
            }
            // 已创建
            // 先调用原函数，驱动粒子位置更新
            pOriginalDrawStuff(pProjectile, flag);

            // 覆写宽度和透明度（因为原函数可能重置这些值）
            // 固定参数：生命周期 4.0，宽度 2.0，透明度 1.0（255/255）
            struct { float lifetime, width, alpha; } trailData = { 4.0f, 2.0f, 1.0f };
            this->pParticleMgr->UpdateParticle(*pEffectIndex, 0x3, &trailData, 0);

            // 已经调用过原函数，不再执行钩子的默认调用
            return MulNX::Hook::Then::Return;
            }).value();

        this->hkFunc_BaseCSGrenadeProjectile_DrawStuff.Attach();

        });

    return true;
}