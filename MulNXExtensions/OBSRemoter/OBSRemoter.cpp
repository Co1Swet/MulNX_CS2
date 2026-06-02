#include "OBSRemoter.hpp"
#include <MulNX/Base/UI/UI.hpp>

void OBSRemoter::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("OBS", this->showWindow);
    if (!this->OBSConnected.load(std::memory_order_acquire)) {
        ImGui::Text(I18n("media.obs.connect.please_try").c_str());
        if (ImGui::Button(I18n("media.obs.connect.try").c_str())) {
            this->ISys().PublishAsync("OBS/CreateConnect"_hash);
        }
        return;
    }
    if (ImGui::Button(I18n("media.obs.record.start").c_str())) {
        this->ISys().PublishAsync("OBS/Record/Start"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("media.obs.record.stop").c_str())) {
        this->ISys().PublishAsync("OBS/Record/Stop"_hash);
    }
    if (ImGui::Button(I18n("media.obs.record.pause").c_str())) {
        this->ISys().PublishAsync("OBS/Record/Pause"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("media.obs.record.resume").c_str())) {
        this->ISys().PublishAsync("OBS/Record/Resume"_hash);
    }
}

bool OBSRemoter::Init() {
    this->showWindow = true;
    // 关闭不需要的日志
    this->client.clear_access_channels(websocketpp::log::alevel::all);
    this->client.init_asio();
    // 设置消息回调
    this->client.set_message_handler([this](websocketpp::connection_hdl hdl, Client::message_ptr msg) {
        this->HandleOBSMsg(hdl, msg);
        });

    this->ISys().SendTask("Update", "MulNXMain", [this]()->bool {
        this->Update();
        return true;
        });

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });

    this->ISys()
        .SubscribeAsync("OBS/CreateConnect")
        .SubscribeAsync("OBS/Record/Start")
        .SubscribeAsync("OBS/Record/Pause")
        .SubscribeAsync("OBS/Record/Resume")
        .SubscribeAsync("OBS/Record/Stop");

    return true;
}

void OBSRemoter::Deinit() {
    if (this->OBSConnected.load(std::memory_order_acquire)) {
        this->client.stop();
    }
}

void OBSRemoter::ProcessMsg(MulNX::Message& msg) {
    return;
    switch (msg.type) {
    case "OBS/CreateConnect"_hash: {
        this->CreateConnect();
        break;
    }
    case "OBS/Record/Start"_hash: {
        this->StartRecording();
        break;
    }
    case "OBS/Record/Pause"_hash: {
        this->PauseRecording();
        break;
    }
    case "OBS/Record/Resume"_hash: {
        this->ResumeRecording();
        break;
    }
    case "OBS/Record/Stop"_hash: {
        this->StopRecording();
        break;
    }
    default:break;
    }
}

void OBSRemoter::CreateConnect() {
    if (this->OBSConnected.load(std::memory_order_acquire)) {
        this->ISys().LogError(I18n("media.obs.connect.cant_again"));
        return;
    }
    websocketpp::lib::error_code ec;
    auto con = client.get_connection(this->OBSURL, ec);
    if (ec) {
        this->ISys().LogError("OBS WebSocket 连接创建失败: " + ec.message());
    }

    this->hdl = con->get_handle();
    this->client.connect(con);
    this->ISys().LogInfo("正在尝试连接 OBS WebSocket...");

    // 启动 OBS 事件循环（独立工作者线程）
    this->ISys().SendTask("client::run", "OBS", [this]() -> bool {
        this->client.run();
        return false;// 结束即卸载任务
        });
}

void OBSRemoter::send_json(const nlohmann::json& json) {
    try {
        std::string msg = json.dump();
        this->client.send(this->hdl, msg, websocketpp::frame::opcode::text);
    }
    catch (const std::exception& e) {
        this->ISys().LogError("发送消息失败: " + std::string(e.what()));
    }
}

void OBSRemoter::send_request(const std::string& requestType) {
    nlohmann::json req;
    req["op"] = 6;
    req["d"]["requestType"] = requestType;
    req["d"]["requestId"] = "media_remoter";
    this->send_json(req);
}
bool OBSRemoter::CheckIsConnected() {
    if (this->OBSConnected.load())return true;
    this->ISys().LogError(I18n("media.obs.connect.error_empty"));
    return false;
}

void OBSRemoter::StartRecording() {
    if (!this->CheckIsConnected())return;
    this->client.get_io_service().post([this]() {this->send_request("StartRecord");});
}

void OBSRemoter::StopRecording() {
    if (!this->CheckIsConnected())return;
    this->client.get_io_service().post([this]() {this->send_request("StopRecord");});
}
void OBSRemoter::PauseRecording() {
    if (!this->CheckIsConnected())return;
    this->client.get_io_service().post([this]() {this->send_request("PauseRecord");});
}
void OBSRemoter::ResumeRecording() {
    if (!this->CheckIsConnected())return;
    this->client.get_io_service().post([this]() {this->send_request("ResumeRecord");});
}

void OBSRemoter::handle_hello(const nlohmann::json& hello) {
    if (hello.contains("d") && hello["d"].contains("authentication")) {
        // TODO: 实现认证字符串生成
    }
    nlohmann::json identify;
    identify["op"] = 1;
    identify["d"]["rpcVersion"] = 1;
    this->send_json(identify);
}

void OBSRemoter::handle_response(const nlohmann::json& response) {
    std::string requestType = response["d"].value("requestType", "");
    if (requestType == "GetVersion") {
        std::string version = response["d"]["responseData"].value("obsVersion", "未知");
        this->ISys().LogSucc("OBS 版本: " + version);
    }
    else if (requestType == "StartRecord") {
        this->ISys().LogSucc("OBS 开始录制");
    }
    else if (requestType == "StopRecord") {
        this->ISys().LogSucc("OBS 停止录制");
    }
    else {
        this->ISys().LogInfo("收到响应，类型: " + requestType);
    }
}

void OBSRemoter::HandleOBSMsg(websocketpp::connection_hdl hdl, Client::message_ptr msg) {
    std::string payload = msg->get_payload();
    try {
        auto json = nlohmann::json::parse(payload);
        int op = json.value("op", -1);

        switch (op) {
        case 0: // Hello
            this->handle_hello(json);
            break;
        case 2: // Identified
            this->ISys().LogSucc(I18n("media.obs.connect.succ"));
            this->OBSConnected.store(true);
            this->send_request("GetVersion");
            break;
        case 5: // Event
            this->ISys().LogInfo("收到 OBS 事件: " + payload);
            break;
        case 7: // RequestResponse
            this->handle_response(json);
            break;
        default:
            this->ISys().LogWarning("收到未知 OpCode: " + std::to_string(op));
            break;
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogError("JSON 解析失败: " + std::string(e.what()));
    }
}