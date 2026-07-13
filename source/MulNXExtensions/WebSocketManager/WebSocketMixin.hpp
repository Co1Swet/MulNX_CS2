#pragma once
#include "WebSocketManager.hpp"

template<typename T>
class WebSocketMixin {
    T* This() { return static_cast<T*>(this); }
    WebSocketManager* pWebSocketManager = nullptr;
protected:
    WebSocketMixin() {
        This()->delayInits->push_back([this]() -> bool {
            this->pWebSocketManager = This()->FindModule<WebSocketManager>("WebSocketManager");
            return true;
            });
    }
    void WebPost(std::string& msg) {
        this->pWebSocketManager->PostWebMsg(msg);
    }
};