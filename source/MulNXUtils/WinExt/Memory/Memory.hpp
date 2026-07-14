#pragma once
#include <MulNX/Common/Exception.hpp>
#include <MulNX/Common/Common.hpp>

#include "DllModule/DllModule.hpp"
#include "Pattern/Pattern.hpp"
#include "Region/Region.hpp"
#include "Assembler/Assembler.hpp"
#include "Hook/Hook.hpp"

#include <cstdint>
#include <atomic>
#include <string>
#include <vector>
#include <optional>
#include <Windows.h>
#include <format>

namespace MulNX {
    namespace Memory {
        namespace ReadWrite {
            class bad_memory_access :public MulNX::Exception {
            public:
                using MulNX::Exception::Exception;
            };

            class bad_memory_read :public bad_memory_access {
            public:
                using bad_memory_access::bad_memory_access;
            };

            class bad_memory_write :public bad_memory_access {
            public:
                using bad_memory_access::bad_memory_access;
            };

            template<MulNX::Pod T>
            bool ReadImpl(uintptr_t address, T& target) {
                __try {
                    target = *reinterpret_cast<T*>(address);
                    return true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return false;
                }
            }

            template<MulNX::Pod T>
            T MRead(uintptr_t address, std::source_location where = std::source_location::current()) {
                T target;
                if (!ReadImpl(address, target)) {
                    throw bad_memory_read(std::format("read error at: 0x{:X}", address), where);
                }
                else {
                    return target;
                }
            }
            template<MulNX::Pod T>
            T MRead(T* address, std::source_location where = std::source_location::current()) {
                T target;
                if (!ReadImpl(reinterpret_cast<uintptr_t>(address), target)) {
                    throw bad_memory_read(std::format("read error at: 0x{:X}", reinterpret_cast<uintptr_t>(address)), where);
                }
                else {
                    return target;
                }
            }

            template<MulNX::Pod T>
            bool WriteImpl(uintptr_t address, const T& value) {
                __try {
                    *reinterpret_cast<T*>(address) = value;
                    return true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return false;
                }
            }

            template<MulNX::Pod T>
            void MWrite(uintptr_t address, const T& value, std::source_location where = std::source_location::current()) {
                if (!WriteImpl(address, value)) {
                    throw bad_memory_write(std::format("write error at: 0x{:X}", address), where);
                }
            }

            template<MulNX::Pod T>
            void MWrite(T* address, const T& value, std::source_location where = std::source_location::current()) {
                if (!WriteImpl(reinterpret_cast<uintptr_t>(address), value)) {
                    throw bad_memory_write(std::format("write error at: 0x{:X}", reinterpret_cast<uintptr_t>(address)), where);
                }
            }
        }
        // 逐字节读取直到遇到空字符
        std::expected<std::string, MulNX::Exception> ReadString(const char* target, std::source_location where = std::source_location::current());
        // 安全读取宽字符串（UTF-16），逐字符读取直到遇到空字符或达到缓冲字符数
        bool ReadWString(const uintptr_t Address, wchar_t* Buffer, size_t BufferCount);
    }
    using namespace Memory::ReadWrite;
}