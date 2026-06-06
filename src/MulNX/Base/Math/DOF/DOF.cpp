#include "DOF.hpp"

#include <algorithm>

void MulNX::Math::CalculateDOFParameters(float FocusDistance, float CrispRadius, float BlurDistance, DOFParam& DOFParam) {
    //确保输入参数有效
    CrispRadius = std::max(0.0f, CrispRadius);
    BlurDistance = std::max(0.0f, BlurDistance);
    FocusDistance = std::max(CrispRadius, FocusDistance); //聚焦距离不能小于清晰半径

    //计算清晰区域的边界
    float NearCrisp = FocusDistance - CrispRadius;
    float FarCrisp = FocusDistance + CrispRadius;

    //计算模糊区域的边界
    float NearBlurry = NearCrisp - BlurDistance;
    float FarBlurry = FarCrisp + BlurDistance;

    //确保值不会为负数
    DOFParam.NearBlurry = std::max(0.0f, NearBlurry);
    DOFParam.NearCrisp = std::max(0.0f, NearCrisp);
    DOFParam.FarCrisp = std::max(0.0f, FarCrisp);
    DOFParam.FarBlurry = std::max(0.0f, FarBlurry);

    //确保逻辑顺序正确（近模糊 <= 近清晰 <= 远清晰 <= 远模糊）
    DOFParam.NearBlurry = std::min(DOFParam.NearBlurry, DOFParam.NearCrisp);
    DOFParam.NearCrisp = std::min(std::max(DOFParam.NearCrisp, DOFParam.NearBlurry),
        DOFParam.FarCrisp);
    DOFParam.FarCrisp = std::max(DOFParam.FarCrisp, DOFParam.NearCrisp);
    DOFParam.FarBlurry = std::max(DOFParam.FarBlurry, DOFParam.FarCrisp);
}