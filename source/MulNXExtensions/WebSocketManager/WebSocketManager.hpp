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
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void Main();
    void Deinit()override;
};