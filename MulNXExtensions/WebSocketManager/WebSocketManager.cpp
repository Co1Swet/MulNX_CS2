#include "WebSocketManager.hpp"
#include <nlohmann/json.hpp>

// 简易 Base64 解码（RFC 4648）
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;
        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

// 从 JSON 字段解析 Base64 编码的 8 字节数据到 uint64_t
uint64_t parse_uint64_from_json_base64(const nlohmann::json& j) {
    if (j.is_string()) {
        std::string b64 = j.get<std::string>();
        auto bytes = base64_decode(b64);
        if (bytes.size() != 8) {
            throw std::runtime_error("Base64 decoded p1/p2 must be exactly 8 bytes");
        }
        uint64_t val;
        std::memcpy(&val, bytes.data(), sizeof(val));
        return val;
    }
    throw std::invalid_argument("p1/p2 must be a base64 string");
}

bool WebSocketManager::Init() {
    this->ISys()
        .SubscribeAsync("WebSocketManager/Post");

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
    this->server.set_message_handler(
        [this](websocketpp::connection_hdl hdl, Server::message_ptr msg) {
            try {

                // 先假设发了一个json
                auto json = nlohmann::json::parse(msg->get_payload());

                auto type = json["type"].get<std::string>();
                uint64_t p1 = parse_uint64_from_json_base64(json["p1"]);
                uint64_t p2 = parse_uint64_from_json_base64(json["p2"]);
                std::string str1 = json["str1"].get<std::string>();
                std::string str2 = json["str2"].get<std::string>();

                auto [mmsg, rp] = MulNX::Message::Create<MulNX::NetExt>(MulNX::HashString(type));
                mmsg.p1.as<uint64_t>() = p1;
                mmsg.p2.as<uint64_t>() = p2;
                rp->str1 = std::move(str1);
                rp->str2 = std::move(str2);

                this->ISys().PublishAsync(std::move(mmsg));

                this->server.send(hdl, "已投入消息总线" + msg->get_payload(), msg->get_opcode());
            }
            catch (const websocketpp::exception& e) {
                this->ISys().LogError("网络回调接口错误: " + *e.what());
            }
        });

    this->SendTask("Web", [this]()->bool {
        try {
            if (!this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return true;
            }
            this->server.listen(this->port);
            this->server.start_accept();
            this->ISys().LogSucc("正在监听端口：" + std::to_string(port));
            // 阻塞调用
            this->server.run();
        }
        catch (const std::exception& e) {
            MulNX::ErrorTerminate("网络功能启动失败！\n" + *e.what());
        }
        return false;
        });
    this->SendTask("MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });
    return true;
}

void WebSocketManager::Deinit() {
    this->server.stop();
}

void WebSocketManager::Main() {
    this->Update();
}

void WebSocketManager::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "WebSocketManager/Post"_hash: {
        MulNX::any_shared_ptr pPayLoad = Msg.asp;
        auto& ios = this->server.get_io_service();
        ios.post([this, pPayLoad]()mutable {

            // 提取要广播的消息内容
            std::string payLoad = std::move(*pPayLoad.get<std::string>());

            // 遍历所有连接并发送
            for (auto hdl : connectionHandles) {
                try {
                    this->server.send(hdl, payLoad, websocketpp::frame::opcode::text);
                }
                catch (const websocketpp::exception& e) {
                    this->ISys().LogError("网络消息发送失败！内容：" + payLoad);
                }
            }

            });

        break;
    }
    }
}