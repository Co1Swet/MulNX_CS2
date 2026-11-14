#pragma once
#include "WebSocketManager.hpp"

template<typename T>
class WebSocketMixin {
    T* This() { return static_cast<T*>(this); }
    WebSocketManager* pWebSocketManager = nullptr;
protected:
    WebSocketMixin() {
        This()->preInits.push_back([this]() {
            this->pWebSocketManager = This()->FindModule<WebSocketManager>();
            return true;
            });
    }
    void WebPost(std::string&& msg) {
        this->pWebSocketManager->PostWebMsg(msg);
    }
};