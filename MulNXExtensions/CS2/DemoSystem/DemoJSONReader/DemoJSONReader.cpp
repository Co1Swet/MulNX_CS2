#include "DemoJSONReader.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

bool DemoJSONReader::Init() {
    this->dirDemos = this->ISys().PathManager()->PathGetForShared("Demos");
    this->ISys().SubscribeAsync("debug/read");

    this->SendTask("DemoSys", [this]()->bool {
        this->Update();
        return true;
        });
    return true;
}

void DemoJSONReader::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "debug/read"_hash: {
        std::string filename = "111.json";
        std::filesystem::path filePath = this->dirDemos / filename;
        if (!std::filesystem::exists(filePath)) {
            this->ISys().LogError("文件不存在: " + filePath.string());
            break;
        }
        try {
            auto json = nlohmann::json::parse(std::ifstream(filePath));
            this->ISys().LogInfo("成功读取 JSON 文件: " + filePath.string());
            // 这里可以根据 JSON 结构进行处理，例如：
        }
        catch (const std::exception& e) {
            this->ISys().LogError("读取 JSON 文件时发生错误: " + filePath.string());
        }
        break;
    }
    }
}