#include "MulNXeCore.hpp"

#include "IPCer/IPCer.hpp"
#include "ConfigManager/ConfigManager.hpp"

#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <MulNX/MulNX.hpp>

//Impl
class MulNXeCoreImpl {
    friend MulNXeCore;
    IPCer IPCer;
    ConfigManager ConfigManager;
};

//各模块接口
MulNXeCore::MulNXeCore() {
    this->pImpl = new MulNXeCoreImpl();
    this->IsRunning = false;
    this->Init();
}
MulNXeCore::~MulNXeCore() {
    delete this->pImpl;
}
IPCer& MulNXeCore::IPCer() {
    return this->pImpl->IPCer;
}
ConfigManager& MulNXeCore::ConfigManager() {
    return this->pImpl->ConfigManager;
}

//系统初始化
bool MulNXeCore::Init() {
    //IPCer初始化
    this->IPCer().Init(this);
    //ConfigManager初始化
    this->ConfigManager().Init(this);
    //加载配置文件
    this->ConfigManager().Config_Load(this->IPCer().PathGet_Config());
    this->IPCer().SetCS2Path(this->ConfigManager().Config_GetCS2Path());

    //输出总成功信息
    this->IsRunning = true;
    return true;
}

void MulNXeCore::VirtualMain() {
    ImGui::Begin("Multiple Next Extension");

    this->IPCer().VirtualMain();

    static std::string error{};
    static bool HasError = false;

    if (!this->IPCer().IOpenedCS2) {
        if (ImGui::Button("打开CS2")) {
            try {
                this->IPCer().OpenCS2();
            }
            catch (std::string e) {
                HasError = true;
                error = e;
            }
        }
    }
    if (HasError) {
        ImGui::Text(error.c_str());
    }

    ImGui::Separator();

    ImGui::Text("欢迎使用 MulNX_CS2");
    ImGui::Text("软件全称：%s", MulNXInfo::FullName);
    ImGui::Text("开发者ID：%s", MulNXInfo::DeveloperName);
    ImGui::Text("当前版本：%s", MulNXInfo::Version);
    ImGui::Text("时间戳：%s", MulNXInfo::TimeStamp);

    ImGui::Separator();
    ImGui::Text("感谢您对MulNX的使用与支持！");

    ImGui::End();
}