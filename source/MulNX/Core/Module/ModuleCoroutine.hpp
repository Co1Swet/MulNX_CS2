#pragma once
#include "ModuleData.hpp"
#include <MulNX/Common/Time.hpp>
#include <MulNX/Common/coroutine.hpp>
#include <map>

namespace MulNX {
    class ModuleCoroutine : public ModuleData {
        friend class ModuleBase;
        friend class Impl_WaitForCondition;
        friend class Impl_WaitForMessage;
        std::vector<std::pair<std::move_only_function<bool()>, std::coroutine_handle<>>> conditionWaiters;
        std::map<MsgType, std::vector<std::pair<std::move_only_function<bool(Message*)>, std::coroutine_handle<>>>> msgWaiters;

        inline static void DestroyCoroutineChain(std::coroutine_handle<MulNX::CoTask::promise_type> handle) {
            if (!handle) return;
            auto next = handle.promise().hContinueCoroutine;
            if (next) {
                DestroyCoroutineChain(next);
            }
            handle.destroy();
        }
    protected:
        ModuleCoroutine() {
            this->preDeinits.push_back([this]() {
                // 销毁所有挂起的条件等待协程
                for (auto& [condition, waiter] : conditionWaiters) {
                    DestroyCoroutineChain(std::coroutine_handle<MulNX::CoTask::promise_type>::from_address(
                        waiter.address()));
                }
                // 销毁所有挂起的消息等待协程
                for (auto& [type, vec] : msgWaiters) {
                    for (auto& [check, h] : vec) {
                        DestroyCoroutineChain(std::coroutine_handle<MulNX::CoTask::promise_type>::from_address(
                            h.address()));
                    }
                }
                return true;
                });
        }
    private:
        struct Impl_WaitForCondition {
            ModuleCoroutine* pModule;
            std::move_only_function<bool()> tempCondition;
            Impl_WaitForCondition(ModuleCoroutine* pModule, std::move_only_function<bool()>&& condition) :
                pModule(pModule), tempCondition(std::move(condition)) {}
            bool await_ready() {
                return this->tempCondition();
            }
            void await_suspend(std::coroutine_handle<> h) {
                this->pModule->conditionWaiters.push_back({ std::move(this->tempCondition), std::move(h) });
            }
            void await_resume() {}
        };
    protected:
        inline auto WaitUntil(std::move_only_function<bool()>&& condition) {
            return Impl_WaitForCondition{ this,std::move(condition) };
        }
    private:
        struct Impl_WaitForMessage {
            ModuleCoroutine* pModule;
            MsgType tempType;
            std::move_only_function<bool(MulNX::Message*)> tempOnUpdate;
            Impl_WaitForMessage(ModuleCoroutine* pModule, MsgType type,
                std::move_only_function<bool(MulNX::Message*)>&& onUpdate) :
                pModule(pModule), tempType(type), tempOnUpdate(std::move(onUpdate)){
                
            }
            bool await_ready() {
                return false;
            }
            void await_suspend(std::coroutine_handle<> h) {
                this->pModule->msgWaiters[this->tempType].push_back({ std::move(this->tempOnUpdate),std::move(h) });
            }
            void await_resume() {}
        };
    protected:
        inline auto WaitMsg(MsgType type, std::move_only_function<bool(MulNX::Message*)>&& onUpdate = nullptr) {
            return Impl_WaitForMessage{ this,type,std::move(onUpdate) };
        }
        inline CoTask WaitMsgForever(MsgType type, Message*& outMsg) {
            auto onUpdate = [&](MulNX::Message* msg)->bool {
                if (msg == nullptr) {
                    return false;
                }
                else {
                    outMsg = msg;
                    return true;
                }
                };
            co_await this->WaitMsg(type, std::move(onUpdate));
            co_return;
        }
        inline CoTask WaitMsgTimed(MsgType type, Message*& outMsg, int millisecond) {
            auto now = std::chrono::system_clock::now();
            auto target = now + std::chrono::milliseconds(millisecond);
            auto onUpdate = [&, target](MulNX::Message* msg) -> bool {
                if (msg == nullptr) {
                    if (target < std::chrono::system_clock::now()) {
                        outMsg = nullptr;
                        return true;
                    }
                    return false;
                }
                else {
                    outMsg = msg;
                    return true;
                }
                };
            co_await this->WaitMsg(type, std::move(onUpdate));
            co_return;
        }
        inline CoTask WaitTimed(bool& flag, const int milliseconds, std::move_only_function<bool()> f) {
            auto start = std::chrono::steady_clock::now();
            auto timeout = std::chrono::milliseconds(milliseconds);
            co_await this->WaitUntil([&]() {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                if (f()) {
                    flag = true;
                    return true;
                }
                if (elapsed >= timeout) {
                    flag = false;
                    return true;
                }
                return false;
                });
        }
        inline CoTask WaitMilliseconds(const int milliseconds) {
            auto start = std::chrono::steady_clock::now();
            auto timeout = std::chrono::milliseconds(milliseconds);
            co_await this->WaitUntil([&]() {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                if (elapsed >= timeout) {
                    return true;
                }
                return false;
                });
        }
    };
}