#pragma once
#include <Game/BaseType.hpp>
#include "CHandle.hpp"

namespace CS2 {

    class C_BaseEntity;
    class CGameSceneNode {
    public:
        C_BaseEntity** m_pOwner() { return Schema<C_BaseEntity*>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_pOwner); }
        CGameSceneNode** m_pChild() { return Schema<CGameSceneNode*>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_pChild); }
        CGameSceneNode** m_pNextSibling() { return Schema<CGameSceneNode*>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_pNextSibling); }

        DirectX::XMFLOAT3* vecAbsOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin); }
        DirectX::XMFLOAT3* vecOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecOrigin); }
        DirectX::XMFLOAT3* angAbsRotation() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_angAbsRotation); }
        DirectX::XMFLOAT3* angRotation() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_angRotation); }
        DirectX::XMFLOAT3* vecWrappedLocalOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecWrappedLocalOrigin); }
        DirectX::XMFLOAT3* angWrappedLocalRotation() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_angWrappedLocalRotation); }
        //DirectX::XMFLOAT3* vRenderOrigin() { return Schema<DirectX::XMFLOAT3>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_vRenderOrigin); }
        bool* bDebugAbsOriginChanges() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::CGameSceneNode::m_bDebugAbsOriginChanges); }
    };

    class CSkeletonInstance;
    class InfoForResourceTypeCModel;
    class CModelState {
    public:
        CSkeletonInstance* GetOwner() { return Schema<CSkeletonInstance>(this, -cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState); }
        
        CStrongHandle<InfoForResourceTypeCModel>* m_hModel() { return Schema<CStrongHandle<InfoForResourceTypeCModel>>(this, cs2_dumper::schemas::client_dll::CModelState::m_hModel); }
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
}