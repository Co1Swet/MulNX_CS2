#pragma once 

#include <stdfloat>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNX/Config/Config.hpp>
#include <MulNXUtils/WinExt/vtable/vtable.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

using GameTime_t = float;

namespace CS2 {

    template<typename T>
    struct C_UtlVectorEmbeddedNetworkVar {
        size_t m_nSize;
        T* m_pData;
        // 后面未知
    };


    enum class ui8TeamNum :uint8_t {
        T = 2,
        CT = 3
    };

    class CViewSetup {
    public:
        // 定位关键数据
        int* pWidth() { return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x434); }
        int* pHeight() { return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x43C); }

        float* pFov() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x498); }
        DirectX::XMFLOAT3* pViewOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + 0x4a0); }
        DirectX::XMFLOAT3* pViewAngles() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + 0x4b8); }
    };

    class CHandleBase {
    public:
        uint32_t value;
        CHandleBase(uint32_t handle) :value(handle) {}
        int GetIndexInEntityList() { return this->value & 0x7FFF; }
        bool Valid() { return this->value != 0xFFFFFFFF; }

        bool operator<(const CHandleBase& rhs) const noexcept {
            return this->value < rhs.value;
        }

        bool operator==(const CHandleBase& rhs) const noexcept {
            return this->value == rhs.value;
        }

        CHandleBase() :value(0xFFFFFFFF) {}
    };

    template<typename T>
    class CHandle :public CHandleBase {
    public:

    };

    class C_ClassInfo {
    public:
        char** pName() { return reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(this) + 0x20); }
    };

    class CGameSceneNode {
    public:
        DirectX::XMFLOAT3* vecAbsOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin); }
        DirectX::XMFLOAT3* vecOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecOrigin); }
        DirectX::XMFLOAT3* angAbsRotation() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_angAbsRotation); }
        DirectX::XMFLOAT3* angRotation() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_angRotation); }
        DirectX::XMFLOAT3* vecWrappedLocalOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecWrappedLocalOrigin); }
        DirectX::XMFLOAT3* angWrappedLocalRotation() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_angWrappedLocalRotation); }
        DirectX::XMFLOAT3* vRenderOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vRenderOrigin); }
        bool* bDebugAbsOriginChanges() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_bDebugAbsOriginChanges); }
    };

    class CModelState {
    public:
        uint64_t* m_MeshGroupMask() { return Schema<uint64_t>(this, cs2_dumper::schemas::client_dll::CModelState::m_MeshGroupMask); }

    };

    class BoneArray {
        constexpr static int32_t unkSize = 32;
    public:
        DirectX::XMFLOAT3* at(int32_t index) { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + index * unkSize); }
    };

    class CSkeletonInstance :public CGameSceneNode {
        constexpr static uintptr_t unkSchema = 0x80;
    public:
        CModelState* m_modelState() { return Schema<CModelState>(this, cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState); }
        BoneArray** unkBoneArray() { return Schema<BoneArray*>(this, cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + unkSchema); }
    };

    class C_BaseEntity :public IVClass {
    public:
        template<typename T>
            requires std::derived_from<T, CS2::C_BaseEntity>
        T* As() { return reinterpret_cast<T*>(this); }

        C_ClassInfo** pClassInfo() { return Schema<C_ClassInfo*>(this, 0x10); }
        CGameSceneNode** pGameSceneNode() { return Schema<CGameSceneNode*>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode); }
        int32_t* iHealth() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth); }
        ui8TeamNum* iTeamNum() { return Schema<ui8TeamNum>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum); }
        CHandle<C_BaseEntity>* m_hEffectEntity() { return Schema<CHandle<C_BaseEntity>>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_hEffectEntity); }
        CHandle<C_BaseEntity>* m_hOwnerEntity() { return Schema<CHandle<C_BaseEntity>>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_hOwnerEntity); }
        GameTime_t* m_flCreateTime() { return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_flCreateTime); }
        DirectX::XMFLOAT3* m_vecAbsVelocity() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_vecAbsVelocity); }
        DirectX::XMFLOAT3* m_vecVelocity() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BaseEntity::m_vecVelocity); }



        DirectX::XMFLOAT3 GetBonePos(int index);
        std::string GetName();

        bool IsPlayerController();
        bool IsPlayerPawn();
    };

    class C_SoundEventEntity :public C_BaseEntity {
    public:

    };

    class C_BaseModelEntity;
    class CGlowProperty {
    public:
        C_BaseModelEntity* GetOwner() { return Schema<C_BaseModelEntity>(this, -cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_Glow); }

        void* fGlowColor() { return Schema<void>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_fGlowColor); }
        int32_t* iGlowType() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_iGlowType); }
        int32_t* iGlowTeam() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_iGlowTeam); }
        int32_t* nGlowRange() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_nGlowRange); }
        int32_t* nGlowRangeMin() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_nGlowRangeMin); }
        void* glowColorOverride() { return Schema<void>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_glowColorOverride); }
        bool* bFlashing() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_bFlashing); }
        float* flGlowTime() { return Schema<float>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_flGlowTime); }
        float* flGlowStartTime() { return Schema<float>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_flGlowStartTime); }
        bool* bEligibleForScreenHighlight() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_bEligibleForScreenHighlight); }
        bool* bGlowing() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CGlowProperty::m_bGlowing); }
    };

    class C_BaseModelEntity :public C_BaseEntity {
    public:


        CGlowProperty* Glow() { return Schema<CGlowProperty>(this, cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_Glow); }
        DirectX::XMFLOAT3* vecViewOffset() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset); }
    };

    class C_BaseClientUIEntity :public C_BaseModelEntity {
    public:
        bool m_bEnabled() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_BaseClientUIEntity::m_bEnabled); }
    };

    class C_PointClientUIHUD :public C_BaseClientUIEntity {
    public:

    };

    class CBaseAnimGraph :public C_BaseModelEntity {
    public:

    };

    class C_PlantedC4 :public CBaseAnimGraph {
    public:

    };

    class C_BaseFlex :public CBaseAnimGraph {
    public:

    };

    class C_BaseCombatCharacter :public C_BaseFlex {
    public:

    };

    class CEconItemAttribute {
    public:
        inline static constexpr size_t ofsize = 0x48;

        uint16_t* m_iAttributeDefinitionIndex() { return Schema<uint16_t>(this, cs2_dumper::schemas::client_dll::CEconItemAttribute::m_iAttributeDefinitionIndex); }
        float* m_flValue() { return Schema<float>(this, cs2_dumper::schemas::client_dll::CEconItemAttribute::m_flValue); }
        float* m_flInitialValue() { return Schema<float>(this, cs2_dumper::schemas::client_dll::CEconItemAttribute::m_flInitialValue); }
        int32_t* m_nRefundableCurrency() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::CEconItemAttribute::m_nRefundableCurrency); }
        bool* m_bSetBonus() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CEconItemAttribute::m_bSetBonus); }
    };

    class CAttributeManager {
    public:

    };

    class CAttributeList {
    public:
        C_UtlVectorEmbeddedNetworkVar<CEconItemAttribute>* m_Attributes() { return Schema<C_UtlVectorEmbeddedNetworkVar<CEconItemAttribute>>(this, cs2_dumper::schemas::client_dll::CAttributeList::m_Attributes); }
        CAttributeManager** m_pManager() { return Schema<CAttributeManager*>(this, cs2_dumper::schemas::client_dll::CAttributeList::m_pManager); }
    };

    class C_EconItemView {
    public:
        uint32_t* m_iItemIDHigh() { return Schema<uint32_t>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_iItemIDHigh); }
        using name161 = char[161];
        name161* m_szCustomName() { return Schema<name161>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_szCustomName); }
        name161* m_szCustomNameOverride() { return Schema<name161>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_szCustomNameOverride); }
        CAttributeList* m_AttributeList() { return Schema<CAttributeList>(this, cs2_dumper::schemas::client_dll::C_EconItemView::m_AttributeList); }
    };



    class C_AttributeContainer :public CAttributeManager {
    public:
        C_EconItemView* m_Item() { return Schema<C_EconItemView>(this, cs2_dumper::schemas::client_dll::C_AttributeContainer::m_Item); }
    };

    class C_EconEntity :public C_BaseFlex {
    public:
        C_AttributeContainer* m_AttributeManager() { return Schema<C_AttributeContainer>(this, cs2_dumper::schemas::client_dll::C_EconEntity::m_AttributeManager); }
        int32_t* m_nFallbackPaintKit() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackPaintKit); }
        int32_t* m_nFallbackSeed() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackSeed); }
        float* m_flFallbackWear() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_EconEntity::m_flFallbackWear); }
        int32_t* m_nFallbackStatTrak() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_EconEntity::m_nFallbackStatTrak); }
    };

    class CPlayerPawnComponent {
    public:

    };

    class C_BasePlayerWeapon :public C_EconEntity {
    public:

    };

    class C_CSWeaponBase :public C_BasePlayerWeapon {
    public:

    };

    class CPlayer_WeaponServices :public CPlayerPawnComponent {
    public:
        CHandle<C_BasePlayerWeapon>* hActiveWeapon() { return reinterpret_cast<CHandle<C_BasePlayerWeapon>*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_WeaponServices::m_hActiveWeapon); }
    };

    using ObserverMode_t = uint32_t;
    class CPlayer_ObserverServices :public CPlayerPawnComponent {
    public:
        uint8_t* iObserverMode() { return Schema<uint8_t>(this, cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode); }
        CHandle<C_BaseEntity>* hObserverTarget() { return Schema<CHandle<C_BaseEntity>>(this, cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_hObserverTarget); }
        ObserverMode_t* iObserverLastMode() { return Schema<ObserverMode_t>(this, cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverLastMode); }
        bool* bForcedObserverMode() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_bForcedObserverMode); }
        float* flObserverChaseDistance() { return Schema<float>(this, cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistance); }
        GameTime_t* flObserverChaseDistanceCalcTime() { return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistanceCalcTime); }
    };

    class CBasePlayerController;
    class C_BasePlayerPawn :public C_BaseCombatCharacter {
    public:
        CPlayer_ObserverServices** pObserverServices() { return Schema<CPlayer_ObserverServices*>(this, cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pObserverServices); }
        DirectX::XMFLOAT3* vOldOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin); }
        //DirectX::XMFLOAT3 GetEyePos(){}
        uint32_t* m_iHideHUD() { return Schema<uint32_t>(this, cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_iHideHUD); }
        CPlayer_WeaponServices** pWeaponServices() { return Schema<CPlayer_WeaponServices*>(this, cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pWeaponServices); }
        CHandle<CBasePlayerController>* m_hController() { return Schema<CHandle<CBasePlayerController>>(this, cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_hController); }

        CHandle<C_BasePlayerWeapon> GetHandleActiveWeapon();
        CHandle<C_BaseEntity> GetHandleObserverTarget();
    };

    class C_CSPlayerPawnBase :public C_BasePlayerPawn {
    public:
        float* m_flFlashBangTime() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_flFlashBangTime); }
        float* m_flFlashScreenshotAlpha() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_flFlashScreenshotAlpha); }
        float* m_flFlashOverlayAlpha() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_flFlashOverlayAlpha); }
        bool* m_bFlashBuildUp() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_bFlashBuildUp); }
        bool* m_bFlashDspHasBeenCleared() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_bFlashDspHasBeenCleared); }
        bool* m_bFlashScreenshotHasBeenGrabbed() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_bFlashScreenshotHasBeenGrabbed); }
        float* m_flFlashMaxAlpha() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_flFlashMaxAlpha); }
        float* m_flFlashDuration() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawnBase::m_flFlashDuration); }
    };


    class EntitySpottedState_t {
    public:
        bool* m_bSpotted() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::EntitySpottedState_t::m_bSpotted); } // bool
        using SpottedMask = uint32_t[2];
        SpottedMask* m_bSpottedByMask() { return Schema<SpottedMask>(this, cs2_dumper::schemas::client_dll::EntitySpottedState_t::m_bSpottedByMask); }  // uint32[2]
    };

    class C_CSPlayerPawn :public C_CSPlayerPawnBase {
    public:
        DirectX::XMFLOAT3* angEyeAngles() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles); }
        EntitySpottedState_t* m_entitySpottedState() { return Schema<EntitySpottedState_t>(this, cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState); }
    };

    class CBasePlayerController :public C_BaseEntity {
    public:

        CHandle<C_BasePlayerPawn>* m_hPawn() { return Schema<CHandle<C_BasePlayerPawn>>(this, cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn); }
        char* m_iszPlayerName() { return Schema<char>(this, cs2_dumper::schemas::client_dll::CBasePlayerController::m_iszPlayerName); } // char[128]
        uint64_t* m_steamID() { return Schema<uint64_t>(this, cs2_dumper::schemas::client_dll::CBasePlayerController::m_steamID); } // uint64
    };

    class CCSPlayerController :public CBasePlayerController {
    public:
        CHandle<C_CSPlayerPawn>* m_hPlayerPawn() { return Schema<CHandle<C_CSPlayerPawn>>(this, cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn); }
    };

    // 源于武器装备继承树
    class C_BaseCSGrenade :public C_CSWeaponBase {
    public:

    };

    class C_BaseGrenade :public C_BaseFlex {
    public:
        CHandle<C_CSPlayerPawn>* m_hThrower() { return Schema<CHandle<C_CSPlayerPawn>>(this, cs2_dumper::schemas::client_dll::C_BaseGrenade::m_hThrower); }

    };

    // 源于飞行投掷物继承树
    class C_BaseCSGrenadeProjectile :public C_BaseGrenade {
    public:
        DirectX::XMFLOAT3* m_vInitialPosition() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_vInitialPosition); }
        DirectX::XMFLOAT3* m_vInitialVelocity() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_vInitialVelocity); }
        int32_t* m_nBounces() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_nBounces); }
        //constexpr std::ptrdiff_t m_nExplodeEffectIndex = 0x13C0; // CStrongHandle<InfoForResourceTypeIParticleSystemDefinition>
        int32_t* m_nExplodeEffectTickBegin() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_nExplodeEffectTickBegin); }
        DirectX::XMFLOAT3* m_vecExplodeEffectOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_vecExplodeEffectOrigin); }
        GameTime_t* m_flSpawnTime() { return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_flSpawnTime); }
        DirectX::XMFLOAT3* vecLastTrailLinePos() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::vecLastTrailLinePos); }
        GameTime_t* flNextTrailLineTime() { return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::flNextTrailLineTime); }
        bool* m_bExplodeEffectBegan() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_bExplodeEffectBegan); }
        bool* m_bCanCreateGrenadeTrail() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_bCanCreateGrenadeTrail); }
        //constexpr std::ptrdiff_t m_nSnapshotTrajectoryEffectIndex = 0x13F0; // ParticleIndex_t
        //constexpr std::ptrdiff_t m_hSnapshotTrajectoryParticleSnapshot = 0x13F8; // CStrongHandle<InfoForResourceTypeIParticleSnapshot>
        //constexpr std::ptrdiff_t m_arrTrajectoryTrailPoints = 0x1400; // CUtlVector<Vector>
        //constexpr std::ptrdiff_t m_arrTrajectoryTrailPointCreationTimes = 0x1418; // CUtlVector<float32>
        float* m_flTrajectoryTrailEffectCreationTime() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_BaseCSGrenadeProjectile::m_flTrajectoryTrailEffectCreationTime); }
    };

    class C_SmokeGrenadeProjectile :public C_BaseCSGrenadeProjectile {
    public:
        int32_t* nSmokeEffectTickBegin() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_nSmokeEffectTickBegin); }
        bool* bDidSmokeEffect() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_bDidSmokeEffect); }
        bool* bSmokeEffectSpawned() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_bSmokeEffectSpawned); }
        DirectX::XMFLOAT3* vSmokeColor() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_vSmokeColor); }

    };
}