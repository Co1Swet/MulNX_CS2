#include "ElementBase.hpp"

#include <fstream>

std::string ElementType_EnumToString(const ElementType Type) {
    switch (Type) {
    case ElementType::ElementBase:return "ElementBase";
    case ElementType::FreeCameraPath:return "FreeCameraPath";

    default:return "None";
    }
}

ElementType ElementType_StringToEnum(const std::string& typeStr) {
    if (typeStr == "FreeCameraPath")  return ElementType::FreeCameraPath;

    else if (typeStr == "ElementBase") return ElementType::ElementBase;
    else return ElementType::None;
}

ElementType ElementBase::TypeGet_Enum()const {
    return this->Type;
}

std::string ElementBase::TypeGet_String()const {
    return ElementType_EnumToString(this->Type);
}

std::string ElementBase::GetBaseInfo()const {
    return I18n("camsys.elem.info_base_fmt", this->Name, this->TypeGet_String(), this->DurationTime);
}

std::string ElementBase::GetMsg()const {
    return I18n("camsys.elem.info_fmt", this->GetBaseInfo(), this->GetPrivateMsg());
}
std::string ElementBase::GetName()const {
    return this->Name;
}
void ElementBase::ResetName(const std::string& NewName) {
    this->Name = NewName;
}

bool ElementBase::CalculateFrame(CameraSystemIO* IO)const {
    if (IO->ElementTime < this->StartTime)return false;
    if (IO->ElementTime > this->EndTime)return false;
    return this->Call(IO);
}

bool ElementBase::DrawBase(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const {
    if (!this->draw) {
        return false;
    }
    return this->Draw(CamDrawer, Matrix, WinWidth, WinHeight);
}

float ElementBase::GetStartTime()const {
    return this->StartTime;
}
float ElementBase::GetEndTime()const {
    return this->EndTime;
}
float ElementBase::GetDurationTime()const {
    return this->DurationTime;
}

std::pair<bool, std::string> ElementBase::Save(const std::filesystem::path& folderPath) {
    if (this->Name.empty())return { false,"元素名为空，无法保存元素到磁盘文件！" };
    std::filesystem::path filePath = folderPath / (this->Name + ".yaml");
    try {
        YAML::Node root;

        root["name"] = this->Name;
        root["type"] = this->TypeGet_String();
        root["duration"] = this->DurationTime;

        auto [ok, msg] = this->SaveImpl(root);
        if (!ok)return { false,std::move(msg) + " 文件路径：" + filePath.string() };

        // 使用Emitter输出，默认块样式
        YAML::Emitter out;
        out << root;

        // 保存到文件
        std::ofstream fout(filePath);
        if (!fout.is_open()) return { false, "无法打开文件进行写入！ 文件路径：" + filePath.string() };

        fout << out.c_str();
        fout.close();

        return { true,"成功保存" + filePath.string() };
    }
    catch (const std::exception& e) {
        return { false, "保存YAML文件时发生错误：" + std::string(e.what()) + " 文件路径：" + filePath.string() };
    }
}