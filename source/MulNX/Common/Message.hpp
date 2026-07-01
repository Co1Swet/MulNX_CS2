#pragma once
#include <MulNX/Common/Common.hpp>
#include <MulNX/Base/any_smart_ptr/any_smart_ptr.hpp>
#include <functional>

namespace MulNX {

    using MsgType = size_t;

    class NetExt {
    public:
        std::string str1;
        std::string str2;
        int64_t timestamp_us = 0; // 微秒，Unix epoch
    };

    class Message {
    public:
        size_t type;
        MulNX::any_shared_ptr asp = nullptr;

        Message() = default;
        Message(size_t Type) : type(Type) {}

        template<typename T, typename... Args>
        static std::pair<Message, T*> Create(size_t type, Args&&... args) {
            Message msg(type);
            auto [p, rp] = MulNX::make_any_shared<T>(std::forward<Args>(args)...);
            msg.asp = std::move(p);
            return std::make_pair(std::move(msg), rp);
        }

    private:
        alignas(8) unsigned char extra[48] = {};

        // 始终返回 tuple<Ts&...>
        template <bool is_const, Pod... Ts>
        auto AccessImpl(std::conditional_t<is_const, const unsigned char*, unsigned char*> storage) noexcept {
            constexpr size_t total_bytes = (sizeof(Ts) + ...);
            static_assert(total_bytes <= sizeof(extra),
                "Access: total size exceeds extra storage (48 bytes)");

            static_assert(((alignof(Ts) <= 8) && ...),
                "Access: alignment of one or more types exceeds storage alignment (8)");

            constexpr auto offsets = ComputeOffsets<Ts...>();

            return[&]<size_t... Is>(std::index_sequence<Is...>) {
                auto make_ref = [&]<size_t I>() -> decltype(auto) {
                    using T = std::tuple_element_t<I, std::tuple<Ts...>>;
                    T* ptr = std::start_lifetime_as<T>(
                        const_cast<unsigned char*>(storage + offsets[I]));
                    if constexpr (is_const)
                        return std::as_const(*ptr);
                    else
                        return *ptr;
                };
                return std::tuple<decltype(make_ref.template operator() < Is > ())...>(
                    make_ref.template operator() < Is > ()...);
            }(std::make_index_sequence<sizeof...(Ts)>{});
        }

    public:
        template <Pod... Ts>
        auto Access() & noexcept {
            return AccessImpl<false, Ts...>(reinterpret_cast<unsigned char*>(extra));
        }

        template <Pod... Ts>
        auto Access() const& noexcept {
            return AccessImpl<true, Ts...>(reinterpret_cast<const unsigned char*>(extra));
        }
    };
    static_assert(sizeof(Message) == 64, "Message size must be 64 bytes");

    using SyncMsgCallback = std::function<void(MulNX::Message& msg)>;
}