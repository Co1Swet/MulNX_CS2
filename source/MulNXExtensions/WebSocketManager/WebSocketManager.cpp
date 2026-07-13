#include "WebSocketManager.hpp"
#include <nlohmann/json.hpp>

bool WebSocketManager::Init() {
    // 关闭它自带的日志功能
    this->server.clear_access_channels(websocketpp::log::alevel::all);
    this->server.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化服务器
    this->server.init_asio();

    this->server.set_open_handler([this](ConnectionHandle handle) {
        this->connectionHandles.insert(handle);
        });

    this->server.set_close_handler([this](ConnectionHandle handle) {
        this->connectionHandles.erase(handle);
        });
    
    // 注册句柄
    this->server.set_message_handler([this](websocketpp::connection_hdl hdl, Server::message_ptr netMsg) {
        this->OnWebMsg(hdl, netMsg);
        });
    
    this->SubscribeSync("System/Init/End", [this](MulNX::Message& msg) {
        auto* pMsgManager = this->FindModule<MulNX::MessageManager>("MessageManager");
        for (auto& [type, meta] : pMsgManager->GetMsgInfo()) {
            if (!meta.isAsync)continue;
            if (!meta.makingHandler)continue;
            this->trans[type] = meta.makingHandler;
        }

        this->SendTask("Main", "MulNXMain", [this]() {
            this->Main();
            return true;
            });

        this->SendTask("server::run", "Web", [this]() {
            try {
                if (!this->pGlobalVars->SystemReady.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    return true;
                }
                this->server.listen(this->port);
                this->server.start_accept();
                this->LogSucc("正在监听端口：" + std::to_string(port));
                // 阻塞调用
                this->server.run();
            }
            catch (const std::exception& e) {
                MulNX::ErrorTerminate("网络功能启动失败！\n" + *e.what());
            }
            return false;
            });
        });
    
    return true;
}

void WebSocketManager::Deinit() {
    this->server.stop();
}

void WebSocketManager::Main() {
    this->Update();
}

void WebSocketManager::OnWebMsg(websocketpp::connection_hdl hdl, Server::message_ptr netMsg) {
    std::string error{};
    try {
        // 先假设发了一个json
        auto json = nlohmann::json::parse(netMsg->get_payload());

        auto type = json["type"].get<std::string>();
        auto payload = json["payload"].get<std::string>();

        auto hash = MulNX::HashString(type);

        auto it = this->trans.find(hash);
        if (it == this->trans.end())throw MulNX::Exception("unregister cmd" + type);

        MulNX::Message msg(hash);
        try {
            it->second(msg, payload);
        }
        catch (std::exception& e) {
            throw MulNX::Exception("trans fail" + *e.what());
        }

        this->PublishAsync(std::move(msg));
        this->server.send(hdl, "已投入消息总线" + netMsg->get_payload(), netMsg->get_opcode());
        return;
    }
    catch (const websocketpp::exception& e) {
        error = std::format("error: {} desc: {}", netMsg->get_payload(), e.what());
    }
    catch (MulNX::Exception& e) {
        error = std::format("error: {} desc: {} on: {}", netMsg->get_payload(), e.what(), e.Where());
    }
    catch (std::exception& e) {
        error = std::format("error: {} desc: {}", netMsg->get_payload(), e.what());
    }
    catch (...) {
        error = std::format("error: {} desc: unknown", netMsg->get_payload());
    }
    this->server.send(hdl, error, netMsg->get_opcode());
    this->LogError(std::move(error));
}

void WebSocketManager::PostWebMsg(std::string& msg) {
    auto& ios = this->server.get_io_service();
    ios.post([this, msg]()mutable {
        // 遍历所有连接并发送
        for (auto hdl : connectionHandles) {
            try {
                this->server.send(hdl, msg, websocketpp::frame::opcode::text);
            }
            catch (const websocketpp::exception& e) {
                this->LogError("网络消息发送失败！内容：" + msg);
            }
        }});
}