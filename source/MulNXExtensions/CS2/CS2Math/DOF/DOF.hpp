#pragma once

namespace MulNX {
    namespace Math {
        // 景深参数包
        class DOFParam {
        public:
            float NearBlurry;
            float NearCrisp;
            float FarCrisp;
            float FarBlurry;
        };
        // 计算景深参数
        void CalculateDOFParameters(float FocusDistance, float CrispRadius, float BlurDistance, DOFParam& DOFParam);
    }
}