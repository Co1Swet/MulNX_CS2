#include "Math.hpp"

#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <sstream>
#include <iomanip>
#include <format>

DirectX::XMFLOAT3 MulNX::Math::CameraKeyframe::GetPosition()const {
	DirectX::XMFLOAT3 position;
	DirectX::XMStoreFloat3(&position, this->PositionAndFOV);
	return position;
}

DirectX::XMFLOAT4 MulNX::Math::CameraKeyframe::GetRotationQuat() const {
	DirectX::XMFLOAT4 Quat;
	DirectX::XMStoreFloat4(&Quat, this->RotationQuat);
	return Quat;
}

float MulNX::Math::CameraKeyframe::GetFOV() const {
	return DirectX::XMVectorGetW(this->PositionAndFOV);
}

DirectX::XMFLOAT3 MulNX::Math::CameraKeyframe::GetRotationEuler()const {
	DirectX::XMFLOAT3 Euler;
	CSQuatToEuler(this->GetRotationQuat(), Euler);
	return Euler;
}

DirectX::XMFLOAT4 MulNX::Math::CameraKeyframe::GetPositionAndFOV()const {
	DirectX::XMFLOAT4 PositionAndFOV;
	DirectX::XMStoreFloat4(&PositionAndFOV, this->PositionAndFOV);
	return PositionAndFOV;
}

MulNX::Math::DOFParam MulNX::Math::CameraKeyframe::GetDOF()const {
    DirectX::XMFLOAT4 dof;
    DirectX::XMStoreFloat4(&dof, this->dof);
    return { dof.x,dof.y,dof.z,dof.w };
}

DirectX::XMVECTOR MulNX::Math::View::ToPositionAndFOV() {
    return DirectX::XMVectorSet(
        this->position.x,
        this->position.y,
        this->position.z,
        this->FOV
    );
}
DirectX::XMVECTOR MulNX::Math::View::ToRotationQuat() {
    return MulNX::Math::CSEulerToQuatVec(this->rotation);
}
DirectX::XMVECTOR MulNX::Math::View::ToDOFPack() {
    return DirectX::XMVectorSet(
        this->dof.NearBlurry,
        this->dof.NearCrisp,
        this->dof.FarCrisp,
        this->dof.FarBlurry
    );
}

void MulNX::Math::ViewBuffer::Push(MulNX::Math::View newView) {
    float factor = this->SMOOTH_FACTOR.load();

    // 1. 验证平滑因子有效性：应为有限值且位于 [0, 1] 区间
    if (!std::isfinite(factor) || factor < 0.0f || factor > 1.0f) {
        return;  // 无效因子，放弃本次更新
    }

    // 2. 验证新视图的位置分量有效性
    if (!std::isfinite(newView.position.x) ||
        !std::isfinite(newView.position.y) ||
        !std::isfinite(newView.position.z)) {
        return;
    }

    // 3. 验证新视图的旋转分量有效性
    if (!std::isfinite(newView.rotation.x) ||
        !std::isfinite(newView.rotation.y) ||
        !std::isfinite(newView.rotation.z)) {
        return;
    }

    // 位置指数平滑
    if (factor > 0.99f) {
        this->view.position = newView.position;
    }
    else {
        this->view.position.x += (newView.position.x - this->view.position.x) * factor;
        this->view.position.y += (newView.position.y - this->view.position.y) * factor;
        this->view.position.z += (newView.position.z - this->view.position.z) * factor;
    }

    // 角度指数平滑（处理环绕）
    auto angleDiff = [](float target, float current) -> float {
        float diff = target - current;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;
        return diff;
        };

    if (factor > 0.99f) {
        this->view.rotation = newView.rotation;
    }
    else {
        this->view.rotation.x += angleDiff(newView.rotation.x, this->view.rotation.x) * factor;
        this->view.rotation.y += angleDiff(newView.rotation.y, this->view.rotation.y) * factor;
        this->view.rotation.z += angleDiff(newView.rotation.z, this->view.rotation.z) * factor;
    }
}

std::string MulNX::Math::CameraKeyframe::GetMsg()const {
    DirectX::XMFLOAT4 PositionAndFOV = this->GetPositionAndFOV();
    DirectX::XMFLOAT3 Euler = this->GetRotationEuler();
    auto dof = this->GetDOF();
    auto msg = I18n("math.camera_keyframe.fmt",
        this->time,
        PositionAndFOV.x, PositionAndFOV.y, PositionAndFOV.z, PositionAndFOV.w,
        Euler.x, Euler.y, Euler.z,
        dof.NearBlurry, dof.NearCrisp, dof.FarCrisp, dof.FarBlurry
    );
    return msg;
}