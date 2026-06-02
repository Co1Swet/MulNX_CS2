#pragma once

#include <MulNX/MulNX.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <functional>

class OBSRemoter final : public MulNX::ModuleBase {
    using Client = websocketpp::client<websocketpp::config::asio>;

    Client client;
    websocketpp::connection_hdl hdl;
    std::string OBSURL = "ws://localhost:4455";

    std::atomic<bool> OBSConnected{ false };
    void CreateConnect();
    

    // 消息处理
    void HandleOBSMsg(websocketpp::connection_hdl hdl, Client::message_ptr msg);

    // 协议层处理
    void handle_hello(const nlohmann::json& hello);
    void handle_response(const nlohmann::json& response);

    // 发送工具
    void send_request(const std::string& requestType);
    void send_json(const nlohmann::json& json);

    bool CheckIsConnected();    
    void StartRecording();
    void StopRecording();
    void PauseRecording();
    void ResumeRecording();

    void Window(MulNX::UINode* node);
public:
    bool Init() override;
    void Deinit()override;
    void ProcessMsg(MulNX::Message& msg)override;
};