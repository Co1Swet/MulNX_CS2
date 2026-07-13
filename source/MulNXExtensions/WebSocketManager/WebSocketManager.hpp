#pragma once
#include <MulNX/MulNX.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>

class WebSocketManager final :public MulNX::Module<WebSocketManager> {
    using Server = websocketpp::server<websocketpp::config::asio>;
    using ConnectionHandle = websocketpp::connection_hdl;
    Server server;
    uint16_t port = 55202;
    std::set<ConnectionHandle, std::owner_less<ConnectionHandle>>connectionHandles;

    std::unordered_map<MulNX::MsgType, std::function<void(MulNX::Message&, std::string_view)>>trans{};
    bool Init()override;
    void Main();
    void Deinit()override;

    void OnWebMsg(websocketpp::connection_hdl hdl, Server::message_ptr netMsg);
public:
    void PostWebMsg(std::string& msg);
};