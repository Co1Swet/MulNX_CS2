#pragma once
#include "IModule.hpp"
#include <MulNX/Common/coroutine.hpp>
#include <map>

namespace MulNX {
    class ModuleCoroutine : public IModule {
        friend class ModuleBase;
        friend class Impl_WaitForCondition;
        friend class Impl_WaitForMessage;
        std::vector<std::pair<std::function<bool()>, std::coroutine_handle<>>>conditionWaiters;
        std::map<MsgType, std::vector<std::pair<std::function<bool(Message*)>, std::coroutine_handle<>>>>msgWaiters;
    protected:
        ModuleCoroutine() {
            this->beforeDeinits->push_back([this]() {
                // 销毁所有挂起的条件等待协程
                for (auto& [condition, waiter] : conditionWaiters) {
                    waiter.destroy();
                }
                // 销毁所有挂起的消息等待协程
                for (auto& [type, vec] : msgWaiters) {
                    for (auto& [check, h] : vec) {
                        h.destroy();
                    }
                }
                return true;
                });
        }
    private:
        struct Impl_WaitForCondition {
            ModuleCoroutine* pModule;
            std::function<bool()> tempCondition;
            Impl_WaitForCondition(ModuleCoroutine* pModule, std::function<bool()>&& condition) :
                pModule(pModule), tempCondition(condition) {}
            bool await_ready() {
                return this->tempCondition();
            }
            void await_suspend(std::coroutine_handle<> h) {
                this->pModule->conditionWaiters.push_back({ std::move(this->tempCondition), std::move(h) });
            }
            void await_resume() {}
        };
    protected:
        auto WaitUntil(std::function<bool()>&& condition) {
            return Impl_WaitForCondition{ this,std::move(condition) };
        }
    private:
        struct Impl_WaitForMessage {
            ModuleCoroutine* pModule;
            MsgType tempType;
            std::function<bool(MulNX::Message*)> tempOnUpdate;
            Impl_WaitForMessage(ModuleCoroutine* pModule, MsgType type,
                std::function<bool(MulNX::Message*)>&& onUpdate) :
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
        auto WaitMsg(MsgType type, std::function<bool(MulNX::Message*)>&& onUpdate = nullptr) {
            return Impl_WaitForMessage{ this,type,std::move(onUpdate) };
        }
        CoTask WaitMsgForever(MsgType type, Message*& outMsg) {
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
        CoTask WaitMsgTimed(MsgType type, Message*& outMsg, int millisecond) {
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
    };
}