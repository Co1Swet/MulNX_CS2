#pragma once

#include <MulNX/Common/Common.hpp>

#include <array>
#include <atomic>

namespace MulNX {
    template <MulNX::Pod T>
    class NewestBuffer {
        std::array<T, 3> slots{};
        std::atomic<int8_t> reading = 0;
        std::atomic<int8_t> nextRead = 1;
        int8_t writing = 2;
    private:
        static int8_t findFree(int8_t r, int8_t n) {
            if (r != n)return 3 - r - n;
            if (r)return 0;
            return 1;
        }

        T* WriteBegin() noexcept {
            auto r = this->reading.load(std::memory_order_acquire);
            auto n = this->nextRead.load(std::memory_order_acquire);
            auto f = findFree(r, n);
            this->writing = f;
            return &this->slots[f];
        }

        void WriteEnd() noexcept {
            this->nextRead.store(this->writing, std::memory_order_release);
        }

        const T* ReadBegin() noexcept {
            auto n = this->nextRead.load(std::memory_order_acquire);
            this->reading.store(n, std::memory_order_release);
            return &this->slots[n];
        }

        void ReadEnd() noexcept {
            // No action required: the swap in ReadBegin already published the previous front
        }
    public:
        class Writer {
            NewestBuffer<T>* pBuffer;
            T* pData;
        private:
            friend NewestBuffer<T>;

            Writer() = delete;
            explicit Writer(NewestBuffer<T>* buffer) : pBuffer(buffer) {
                this->pData = this->pBuffer->WriteBegin();
            }

            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;
            Writer(Writer&&) = delete;
            Writer& operator=(Writer&&) = delete;
        public:
            T* operator->() { return this->pData; }
            T& operator*() { return *this->pData; }

            ~Writer() {
                this->pBuffer->WriteEnd();
            }
        };

        class Reader {
            NewestBuffer<T>* pBuffer;
            const T* pData;
        private:
            friend NewestBuffer<T>;
            explicit Reader(NewestBuffer<T>* buffer) : pBuffer(buffer) {
                this->pData = pBuffer->ReadBegin();
            }
        public:
            const T* operator->() const { return this->pData; }
            const T& operator*() const { return *this->pData; }

            ~Reader() {
                this->pBuffer->ReadEnd();
            }
        };

        NewestBuffer() = default;

        // 禁止拷贝和移动（内部有原子变量）
        NewestBuffer(const NewestBuffer&) = delete;
        NewestBuffer& operator=(const NewestBuffer&) = delete;
        NewestBuffer(NewestBuffer&&) = delete;
        NewestBuffer& operator=(NewestBuffer&&) = delete;

        friend class Writer;
        friend class Reader;

        Writer Write() {
            return Writer(this);
        }
        Reader Read() {
            return Reader(this);
        }
    };
}
