#include "InputSystem.hpp"
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>
#include <Windows.h>

bool MulNX::InputSystem::Init() {
    this->SendTask("UpdateKeysState", "InputSystem", [this]()->bool {
        this->UpdateKeysState();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return true;
        });
    return true;
}
void MulNX::InputSystem::SetInputWindow(void* window) {
    this->ResetInput();
    this->InputWindow = window;
    this->LimitToForegroundWindow = true;
}
void MulNX::InputSystem::ResetInput() {
    this->InputReset++;
}
bool MulNX::InputSystem::IsInputActive()const {
    return !this->LimitToForegroundWindow ||
        (this->InputReady && this->InputReset == this->ProcessedInputReset &&
            this->InputWindow && GetForegroundWindow() == this->InputWindow);
}
bool MulNX::InputSystem::UpdateKeysState() {
    unsigned int Threshold = this->Threshold;
    unsigned int DThreshold = Threshold * 2;
    this->CurrentTimeMs = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->ClockEpoch).count());
    auto reset = this->InputReset.load();
    bool active = !this->LimitToForegroundWindow ||
        (this->InputWindow && GetForegroundWindow() == this->InputWindow);
    if (!active || reset != this->ProcessedInputReset) {
        this->InputReady = false;
        active = false;
    }
    for (int vkCode = 0; vkCode < 256; ++vkCode) {
        SHORT keyState = GetAsyncKeyState(vkCode);
        bool pressed = (keyState & 0x8000) != 0;
        this->KeysState[vkCode].Current = active && pressed;
        if (!active) {
            this->KeysState[vkCode].ComboClick = 0;
            this->KeysState[vkCode].BufferComboClick = 0;
            this->KeysState[vkCode].Previous = pressed;
            continue;
        }
        //this->KeysState[vkCode].Current = (keyboardState[vkCode] & 0x80) != 0;
        if (this->KeysState[vkCode].BufferComboClick) {
            if (this->CurrentTimeMs - this->KeysState[vkCode].LastPressTimeMs > DThreshold) {
                this->KeysState[vkCode].BufferComboClick = 0;
            }
        }
        if (this->KeysState[vkCode].ComboClick) {
            if (this->CurrentTimeMs - this->KeysState[vkCode].LastPressTimeMs > Threshold) {
                this->KeysState[vkCode].BufferComboClick.store(this->KeysState[vkCode].ComboClick.load());
                this->KeysState[vkCode].ComboClick = 0;
            }
        }
        if (this->KeysState[vkCode].Current) {
            if (!this->KeysState[vkCode].Previous) {
                this->KeysState[vkCode].ComboClick++;
            }
            this->KeysState[vkCode].Previous = true;
            this->KeysState[vkCode].LastPressTimeMs = this->CurrentTimeMs;
        }
        else {
            this->KeysState[vkCode].Previous = false;
        }
    }
    this->ProcessedInputReset = reset;
    if (active)this->InputReady = true;
    return true;
}

bool MulNX::InputSystem::IsKeyPressed(const unsigned char vkCode)const {
    if (!this->IsInputActive())return false;
    auto state = this->KeysState[vkCode].Current.load(std::memory_order_acquire);
    return state;//返回指定键的当前状态
}
bool MulNX::InputSystem::IsKeyPackPressed(const KeyCheckPack& Pack)const {
    if (!Pack.Usable)return false;
    // 检查Ctrl状态必须完全匹配
    bool ctrlPressed = this->IsKeyPressed(VK_CONTROL) ||
        this->IsKeyPressed(VK_LCONTROL) ||
        this->IsKeyPressed(VK_RCONTROL);
    if (Pack.Ctrl != ctrlPressed) return false;

    // 检查Shift状态必须完全匹配
    bool shiftPressed = this->IsKeyPressed(VK_SHIFT) ||
        this->IsKeyPressed(VK_LSHIFT) ||
        this->IsKeyPressed(VK_RSHIFT);
    if (Pack.Shift != shiftPressed) return false;

    // 检查Alt状态必须完全匹配
    bool altPressed = this->IsKeyPressed(VK_MENU) ||
        this->IsKeyPressed(VK_LMENU) ||
        this->IsKeyPressed(VK_RMENU);
    if (Pack.Alt != altPressed) return false;

    auto state = this->IsKeyPressed(Pack.vkCode);
    return state;
}

bool MulNX::InputSystem::CheckComboClick(const unsigned char vkCode, unsigned char TargetCombo) {
    if (!this->IsInputActive())return false;
    if (TargetCombo < 0)return false;
    return this->KeysState[vkCode].BufferComboClick.compare_exchange_strong(TargetCombo, 0);
}

//要求按键检测包与当前状态完全匹配
bool MulNX::InputSystem::CheckWithPack(const KeyCheckPack& Pack) {
    if (!Pack.Usable)return false;
    // 检查Ctrl状态必须完全匹配
    bool ctrlPressed = this->IsKeyPressed(VK_CONTROL) ||
        this->IsKeyPressed(VK_LCONTROL) ||
        this->IsKeyPressed(VK_RCONTROL);
    if (Pack.Ctrl != ctrlPressed) return false;

    // 检查Shift状态必须完全匹配
    bool shiftPressed = this->IsKeyPressed(VK_SHIFT) ||
        this->IsKeyPressed(VK_LSHIFT) ||
        this->IsKeyPressed(VK_RSHIFT);
    if (Pack.Shift != shiftPressed) return false;

    // 检查Alt状态必须完全匹配
    bool altPressed = this->IsKeyPressed(VK_MENU) ||
        this->IsKeyPressed(VK_LMENU) ||
        this->IsKeyPressed(VK_RMENU);
    if (Pack.Alt != altPressed) return false;

    return this->CheckComboClick(Pack.vkCode, Pack.ComboClick);
}

unsigned char MulNX::InputSystem::CheckComboClickUnremove(const unsigned char vkCode)const {
    if (!this->IsInputActive())return 0;
    return this->KeysState[vkCode].ComboClick;
}

void MulNX::InputSystem::ResetThreshold(const unsigned int Threshold) {
    if (Threshold < 1)return;
    this->Threshold = Threshold;
}