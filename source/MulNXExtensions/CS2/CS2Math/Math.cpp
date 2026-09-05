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