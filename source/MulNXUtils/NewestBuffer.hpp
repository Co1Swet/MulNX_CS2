#pragma once
#include <MulNX/Common/Type.hpp>
#include <mutex>

namespace MulNX {
    template <MulNX::Pod T>
    class NewestBuffer {
        T latest{};
        std::mutex mutex;
    private:
        T ReadSnapshot() {
            std::lock_guard lock(this->mutex);
            return this->latest;
        }

        void Publish(const T& value) {
            std::lock_guard lock(this->mutex);
            this->latest = value;
        }
    public:
        class Writer {
            NewestBuffer<T>* pBuffer;
            T data;
        private:
            friend NewestBuffer<T>;

            Writer() = delete;
            explicit Writer(NewestBuffer<T>* buffer) :
                pBuffer(buffer), data(buffer->ReadSnapshot()) {}

            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;
            Writer(Writer&&) = delete;
            Writer& operator=(Writer&&) = delete;
        public:
            T* operator->() { return &this->data; }
            T& operator*() { return this->data; }

            ~Writer() {
                this->pBuffer->Publish(this->data);
            }
        };

        class Reader {
            const T data;
        private:
            friend NewestBuffer<T>;
            explicit Reader(NewestBuffer<T>* buffer) : data(buffer->ReadSnapshot()) {}
        public:
            const T* operator->() const { return &this->data; }
            const T& operator*() const { return this->data; }
        };

        NewestBuffer() = default;

        // 禁止拷贝和移动（内部有互斥锁）
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
