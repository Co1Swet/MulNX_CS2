#pragma once

#include <cstdint>
#include <utility>  // for std::forward
#include <type_traits>

// 辅助 trait：将函数类型 F = R(Args...) 转换为对应的带显式 this 指针的函数指针类型 R(*)(void*, Args...)
template<typename F>
struct FuncPtrWithThis;

template<typename R, typename... Args>
struct FuncPtrWithThis<R(Args...)> {
    using type = R(*)(void*, Args...);
};

// VExecuter 主模板
template<typename F>
class VExecutor;

// VExecuter 模板类：包装对象指针和函数指针，提供 operator() 直接调用
template<typename R, typename... Args>
class VExecutor<R(Args...)> {
private:
    void* pClass = nullptr;
    R(*pFunc)(void*, Args...) = nullptr;
public:
    VExecutor() = default;
    // 构造函数
    VExecutor(void* obj, R(*func)(void*, Args...)) : pClass(obj), pFunc(func) {}

    // 调用操作符：直接使用参数，自动传入对象指针
    // 参数按原样传递，如果函数参数包含引用，则保持引用语义
    R operator()(Args... args) const {
        return pFunc(pClass, std::forward<Args>(args)...);
    }

    // 重置对象和函数指针
    void reset(void* obj = nullptr, R(*func)(void*, Args...) = nullptr) {
        pClass = obj;
        pFunc = func;
    }

    // 检查是否有效
    explicit operator bool() const { return pClass != nullptr && pFunc != nullptr; }

    auto GetRawFuncPtr() const -> R(*)(void*, Args...) {
        return pFunc;
    }
};

// IVClass: 工具类，用于从对象指针获取虚表信息
// 使用时将目标对象指针强制转换为 IVClass*，然后调用其方法
class IVClass {
public:
    // 获取虚表指针（vptr）的值
    uintptr_t GetVTablePtr() {
        // 对象首地址处存放着指向虚函数表的指针
        return *reinterpret_cast<uintptr_t*>(this);
    }

    // 获取虚函数表中第 index 个函数的地址（原始整数形式）
    uintptr_t GetVFuncPtr(int index) {
        uintptr_t vptr = GetVTablePtr();
        uintptr_t* vtable = reinterpret_cast<uintptr_t*>(vptr);
        return vtable[index];  // 返回函数入口地址
    }

    // 返回一个可调用对象 VExecuter，支持函数类型语法
    // 用法: auto func = obj->GetFunc<void(int, const char*)>(49);
    template<typename F>
    auto GetVFunc(size_t index) {
        using FuncPtrType = typename FuncPtrWithThis<F>::type;
        auto funcPtr = reinterpret_cast<FuncPtrType>(this->GetVFuncPtr(index));
        return VExecutor<F>(this, funcPtr);
    }

    template<typename T>
    static IVClass* Assume(T* pClass) {
        return reinterpret_cast<IVClass*>(const_cast<std::remove_cv_t<T>*>(pClass));
    }

    static IVClass* Assume(uintptr_t pClass) {
        return reinterpret_cast<IVClass*>(pClass);
    }
};