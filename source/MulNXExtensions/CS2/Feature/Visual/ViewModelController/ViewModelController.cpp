#include "ViewModelController.hpp"

using GetViewModelInfo_t = void(*)(void* rcx, float* pViewmodelOffsets, float* pFov);
using GetHandSide_t = bool(*)(void* rcx);

void ViewModelController::Menu() {
    ImGui::SeparatorText("ViewModel");
    if (ImGui::Button("启用所有")) {
        this->PublishAsync("ViewModel/EnableAll"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("禁用所有")) {
        this->PublishAsync("ViewModel/DisableAll"_hash);
    }
    MulNX::UI::Checkbox("X覆盖", this->enableX);
    MulNX::UI::SliderFloat("X值", this->offsetX, -10.0f, 10.0f);
    MulNX::UI::Checkbox("Y覆盖", this->enableY);
    MulNX::UI::SliderFloat("Y值", this->offsetY, -10.0f, 10.0f);
    MulNX::UI::Checkbox("Z覆盖", this->enableZ);
    MulNX::UI::SliderFloat("Z值", this->offsetZ, -10.0f, 10.0f);

    MulNX::UI::Checkbox("Fov覆盖", this->enableFov);
    MulNX::UI::SliderFloat("模型FOV", this->myFov, 30.0f, 150.0f);

    MulNX::UI::Checkbox("左右持枪覆盖", this->enableHandSide);
    MulNX::UI::Checkbox("左手持枪", this->isLeftHandSide);

    ImGui::Separator();
}

void ViewModelController::MenuPlayer(MulNX::Message* umsg) {
    // auto&& [uid] = umsg->Access<Steam64UID>();
    // auto pPawn = this->CS2->client
    // auto pPawn = this->CS2->client.TryGetObservingPawn();
    // if (!pPawn)return;
    // auto pCtrler = this->CS2->client.GetBaseEntityFromHandle(MulNX::MRead(pPawn->m_hController()))
    //     ->As<CS2::CCSPlayerController>();
    // if (!pCtrler)return;


    
    // if (ImGui::Button("复制内存准星到剪贴板")) {
    //     ImGui::SetClipboardText(str.c_str());
    // }
}

bool ViewModelController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {

        auto tGetViewModelInfo = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::ViewModel::Func_GetViewModelInfo).Data();
        this->hkGetViewModelInfo = MulNX::Hook::Create(tGetViewModelInfo, [this](MulNX::Hook* hk, RegContext* ctx) {
            float* pOffsets = std::bit_cast<float*>(ctx->rdx);
            float* pFov = std::bit_cast<float*>(ctx->r8);
            hk->CallMaybeAs<GetViewModelInfo_t>((void*)ctx->rcx, pOffsets, pFov);
            if (this->enableX.load(std::memory_order_acquire))
                pOffsets[0] = this->offsetX.load(std::memory_order_acquire);
            if (this->enableY.load(std::memory_order_acquire))
                pOffsets[1] = this->offsetY.load(std::memory_order_acquire);
            if (this->enableZ.load(std::memory_order_acquire))
                pOffsets[2] = this->offsetZ.load(std::memory_order_acquire);

            if (this->enableFov.load(std::memory_order_acquire))
                *pFov = this->myFov.load(std::memory_order_acquire);

            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkGetViewModelInfo, "GetViewModelInfo");

        auto tGetIfHandLeftSide = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::ViewModel::Func_GetIfHandLeftSide).Data();
        this->hkGetIfHandLeftSide = MulNX::Hook::Create(tGetIfHandLeftSide, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->rax = hk->CallMaybeAs<GetHandSide_t>((void*)ctx->rcx);
            if (this->enableHandSide.load(std::memory_order_acquire))
                ctx->rax = this->isLeftHandSide.load(std::memory_order_acquire);
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkGetIfHandLeftSide, "GetIfHandLeftSide");

        });

    (*this)
        .SubscribeAsync<void>("ViewModel/EnableAll")
        .SubscribeAsync<void>("ViewModel/DisableAll")
        ;

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) {this->Menu();});
    this->UIRegisterCallback("UI.Player.Info", [this](auto uico, auto umsg) {
        try {
            this->MenuPlayer(umsg);
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
        }
        });

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void ViewModelController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "ViewModel/EnableAll"_hash: {
        this->enableX = true;
        this->enableY = true;
        this->enableZ = true;
        this->enableFov = true;
        this->enableHandSide = true;
        break;
    }
    case "ViewModel/DisableAll"_hash: {
        this->enableX = false;
        this->enableY = false;
        this->enableZ = false;
        this->enableFov = false;
        this->enableHandSide = false;
        break;
    }
    default:break;
    }
}