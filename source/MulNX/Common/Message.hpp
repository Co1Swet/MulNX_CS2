#pragma once

#include <MulNX/Common/Common.hpp>
#include <MulNX/Base/any_smart_ptr/any_smart_ptr.hpp>
#include <functional>
#include <cstdint>

namespace MulNX {
    using MsgType = size_t;

    class NetExt {
    public:
        std::string str1;
        std::string str2;
        int64_t timestamp_us = 0; // 微秒，Unix epoch
    };
    // MulNX消息，8+8+8+8（asp8字节实现），32对齐，半个Cache Line
    class Message {
        class Param {
            uint64_t internal;
        public:
            template<MulNX::PodSizeIn<4, 8> T>
            T& as() {
                return reinterpret_cast<T&>(internal);
            }
            template<MulNX::PodSizeIn<0, 4> T>
            T& low() {
                return reinterpret_cast<T&>(*reinterpret_cast<uint32_t*>(&internal));
            }
            template<MulNX::PodSizeIn<0, 4> T>
            T& high() {
                return reinterpret_cast<T&>(*reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(&internal) + 4));
            }
        };
        static_assert(alignof(Param) == 8, "Param 未按 8 字节对齐");
    public:
        // 消息类型，用于区分消息
        size_t type;
        Param p1;
        Param p2;
        // 8字节类型安全擦除共享指针，使用时需用get方法正确恢复，类型错误返回nullptr
        MulNX::any_shared_ptr asp = nullptr;
        Message() = default;
        Message(size_t Type) :type(Type) {}
        template<typename T, typename... Args>
        static std::pair<Message, T*> Create(size_t type, Args&&... args) {
            Message msg(type);
            auto [p, rp] = MulNX::make_any_shared<T>(std::forward<Args>(args)...);
            msg.asp = std::move(p);
            return std::make_pair(std::move(msg), rp);
        }
    };

    using SyncMsgCallback = std::function<void(MulNX::Message& msg)>;
}