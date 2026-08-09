#include"AutoCfgLoad.hpp"

bool AutoCfgLoad::Init() {
    this->SubscribeSync("Hook/Present/First", [this](MulNX::Message& msg) {
        auto dir = this->PathGet("GameLaunch");
        this->FireAsyncCfg(dir);
        });

    (*this)
        .SubscribeAsync("Demo/SetOperating")
        .SubscribeAsync<void>("CSCfg/User1")
        .SubscribeAsync<void>("CSCfg/User2")
        .SubscribeAsync<void>("CSCfg/Tournament")
        .SubscribeAsync<void>("CSCfg/POV")
        .SubscribeAsync<void>("CSCfg/Match")
        ;

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void AutoCfgLoad::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/SetOperating"_hash: {
        static bool first = true;
        if (first) {
            first = false;
            this->FireAsyncCfg(this->PathGet("FirstPlayDemo"));
        }
        break;
    }
    case "CSCfg/User1"_hash: {
        this->FireAsyncCfg(this->PathGet("User1"));
        break;
    }
    case "CSCfg/User2"_hash: {
        this->FireAsyncCfg(this->PathGet("User2"));
        break;
    }
    case "CSCfg/Tournament"_hash: {
        this->FireAsyncCfg(this->PathGet("Tournament"));
        break;
    }
    case "CSCfg/POV"_hash: {
        this->FireAsyncCfg(this->PathGet("POV"));
        break;
    }
    case "CSCfg/Match"_hash: {
        this->FireAsyncCfg(this->PathGet("Match"));
        break;
    }
    }
}

void AutoCfgLoad::FireAsyncCfg(const std::filesystem::path& dir)const {
    namespace fs = std::filesystem;

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        this->LogError(std::format("目录不存在或不是有效目录：{}", dir.string()));
        return;
    }

    this->LogInfo(std::format("正在加载来自 {} 的 Cfg(s)", dir.string()));

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".cfg")
            continue;

        std::string filePath = entry.path().string();
        this->LogInfo(std::format("正在加载配置文件：{}", filePath));

        std::ifstream file(entry.path());
        if (!file.is_open()) {
            this->LogWarning(std::format("无法打开文件：{}", filePath));
            continue;
        }

        std::string line;
        while (std::getline(file, line)) {
            // 忽略空行和注释（以 # 或 // 开头）
            if (line.empty() || line.starts_with("#") || line.starts_with("//"))
                continue;
            // 异步执行该行命令
            this->AsyncCommand(std::move(line));
        }

        file.close();
        this->LogSucc(std::format("配置文件 {} 已加载完毕！", filePath));
    }

    this->LogInfo(std::format("所有 .cfg 文件已处理完成。"));
}