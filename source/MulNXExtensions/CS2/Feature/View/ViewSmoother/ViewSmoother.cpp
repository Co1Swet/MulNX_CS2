#include "ViewSmoother.hpp"
#include <MulNX/Base/UI/UI.hpp>

void ViewBuffer::Push(MulNX::Math::View&& newView) {
    float factor = this->smooth_buffer.load();

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

void ViewSmoother::Menu() {
    MulNX::UI::Checkbox("启用视角平滑", this->enable);
    MulNX::UI::SliderFloat("指数平滑系数（越低越平滑）", this->buffer.smooth_buffer, 0.0f, 1.0f);
}

bool ViewSmoother::Init() {
    this->UIRegisterCallback("UI.CameraSetting", [this](auto&&...) {this->Menu();});

    return true;
}

bool ViewSmoother::HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) {
    if (!this->enable.load(std::memory_order_acquire))return false;

    MulNX::Math::View view;
    view.position = *viewSetup->pViewOrigin();
    view.rotation = *viewSetup->pViewAngles();

    this->buffer.Push(std::move(view));
    auto out = this->buffer.Get();

    *viewSetup->pViewOrigin() = out.position;
    *viewSetup->pViewAngles() = out.rotation;

    return true;
}