#pragma once

#include <memory>
#include <typeinfo>
#include <utility>
#include <typeindex>
#include <atomic>

namespace MulNX {
    namespace detail {
        class any_base {
        public:
            virtual ~any_base() = default;

            virtual const std::type_info& type() const = 0;
            virtual void* data() noexcept = 0;
            virtual const void* data() const noexcept = 0;
            //virtual any_base* clone() const = 0;

            any_base() = default;
            any_base(any_base&&) noexcept = default;
            any_base& operator=(any_base&&) noexcept = default;

            any_base(const any_base&) = default;
            any_base& operator=(const any_base&) = default;
        };

        template<typename T>
        class any_derived : public any_base {
            T value_;
        public:
            template<typename... Args>
            any_derived(Args&&... args)
                : value_(std::forward<Args>(args)...) {}

            const std::type_info& type() const override {
                return typeid(T);
            }

            void* data() noexcept override {
                return &value_;
            }

            const void* data() const noexcept override {
                return &value_;
            }

            T& get() { return value_; }
            const T& get() const { return value_; }
        };
    } // namespace detail

    class any_unique_ptr {
        detail::any_base* ptr_;

    public:
        any_unique_ptr() noexcept : ptr_(nullptr) {}
        explicit any_unique_ptr(detail::any_base* ptr) noexcept : ptr_(ptr) {}

        //添加一个用nullptr_t构造的构造函数
        any_unique_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

        any_unique_ptr(any_unique_ptr&& other) noexcept
            : ptr_(other.release()) {}

        any_unique_ptr& operator=(any_unique_ptr&& other) noexcept {
            if (this != &other) {
                reset(other.release());
            }
            return *this;
        }

        any_unique_ptr(const any_unique_ptr&) = delete;
        any_unique_ptr& operator=(const any_unique_ptr&) = delete;

        ~any_unique_ptr() {
            reset();
        }

        detail::any_base* release() noexcept {
            detail::any_base* temp = ptr_;
            ptr_ = nullptr;
            return temp;
        }

        template<typename T>
        std::unique_ptr<T> release_unique() noexcept {
            if (!ptr_ || ptr_->type() != typeid(T)) return nullptr;
            auto* derived = static_cast<detail::any_derived<T>*>(ptr_);
            T* p = new T(std::move(derived->get()));
            delete ptr_;
            ptr_ = nullptr;
            return std::unique_ptr<T>(p);
        }

        void reset(detail::any_base* ptr = nullptr) noexcept {
            if (ptr_ != ptr) {
                delete ptr_;
                ptr_ = ptr;
            }
        }

        void swap(any_unique_ptr& other) noexcept {
            std::swap(ptr_, other.ptr_);
        }

        detail::any_base* get() noexcept { return ptr_; }
        const detail::any_base* get() const noexcept { return ptr_; }

        template<typename T>
        T* get() {
            if (!ptr_) return nullptr;
            if (ptr_->type() != typeid(T)) return nullptr;
            auto* derived = static_cast<detail::any_derived<T>*>(ptr_);
            return &derived->get();
        }

        template<typename T>
        const T* get() const {
            if (!ptr_) return nullptr;
            if (ptr_->type() != typeid(T)) return nullptr;
            auto* derived = static_cast<const detail::any_derived<T>*>(ptr_);
            return &derived->get();
        }

        template<typename T>
        T* get_unsafe() const {
            if (!ptr_) return nullptr;
            auto* derived = reinterpret_cast<detail::any_derived<T>*>(ptr_);
            if (derived) return &derived->get();
            return nullptr;
        }

        explicit operator bool() const noexcept {
            return ptr_ != nullptr;
        }

        const std::type_info& type() const noexcept {
            return ptr_ ? ptr_->type() : typeid(void);
        }

        /*any_unique_ptr clone() const {
            if (ptr_) {
                return any_unique_ptr(ptr_->clone());
            }
            return any_unique_ptr();
        }*/
    };

    class ControlPack {
    public:
        std::atomic<size_t> ref_count;
        std::atomic<size_t> weak_count;
        detail::any_base* ptr_;
    };

    class any_shared_ptr {
        friend class any_weak_ptr;
    private:
        ControlPack* control_;
    public:
        any_shared_ptr() noexcept : control_(nullptr) {}
        explicit any_shared_ptr(detail::any_base* ptr, ControlPack* control = nullptr)
            : control_(control) {
            if (control_ == nullptr) {
                control_ = new ControlPack();
                control_->ref_count.store(1);
                control_->ptr_ = ptr;
            }
        }
        any_shared_ptr(std::nullptr_t) noexcept : control_(nullptr) {}

        any_shared_ptr(const any_shared_ptr& other) noexcept
            :  control_(other.control_) {
            if (control_) {
                control_->ref_count.fetch_add(1);
                control_->ptr_ = other.control_->ptr_;
            }
        }
        any_shared_ptr& operator=(const any_shared_ptr& other) noexcept {
            if (this != &other) {
                // Decrease current object's ref count
                if (control_ && control_->ref_count.fetch_sub(1) == 1) {
                    delete control_->ptr_;
                    if (control_->weak_count.load() == 0) {
                        delete control_;
                    }
                }
                // Copy from other
                control_ = other.control_;
                if (control_) {
                    control_->ref_count.fetch_add(1);
                }
            }
            return *this;
        }
        any_shared_ptr(any_shared_ptr&& other) noexcept
            : control_(other.control_) {
            other.control_ = nullptr;
        }
        any_shared_ptr& operator=(any_shared_ptr&& other) noexcept {
            if (this != &other) {
                // Move from other
                control_ = other.control_;
                other.control_ = nullptr;
            }
            return *this;
        }

        ~any_shared_ptr() {
            if (control_ && control_->ref_count.fetch_sub(1) == 1) {
                delete control_->ptr_;
                if (control_->weak_count.load() == 0) {
                    delete control_;
                }
            }
        }

        explicit operator bool() const noexcept {
            return control_ != nullptr && control_->ptr_ != nullptr;
        }

        template<typename T>
        T* get() {
            if (!control_)return nullptr;
            if (!control_->ptr_) return nullptr;
            if (control_->ptr_->type() != typeid(T)) return nullptr;
            auto* derived = static_cast<detail::any_derived<T>*>(control_->ptr_);
            return &derived->get();
        }

        template<typename T>
        T* reinterpret_get() {
            if (!control_)return nullptr;
            if (!control_->ptr_) return nullptr;
            auto* derived = static_cast<detail::any_derived<T>*>(control_->ptr_);
            return &derived->get();
        }
    };

    // class any_weak_ptr {
    // private:
    //     detail::any_base* ptr_;
    //     ControlPack* control_;
    // public:
    //     any_weak_ptr() noexcept : ptr_(nullptr), control_(nullptr) {}
    //     /*explicit any_weak_ptr(detail::any_base* ptr, ControlPack* control = nullptr)
    //         : ptr_(ptr), control_(control) {
    //         if (control_) {
    //             control_->weak_count.fetch_add(1, std::memory_order_relaxed);
    //         }
    //     }*/
    //     any_weak_ptr(std::nullptr_t) noexcept : ptr_(nullptr), control_(nullptr) {}

    //     any_weak_ptr(const any_shared_ptr& shared) noexcept
    //         : ptr_(shared.ptr_), control_(shared.control_) {
    //         if (control_) {
    //             control_->weak_count.fetch_add(1, std::memory_order_relaxed);
    //         }
    //     }
    //     any_weak_ptr(const any_weak_ptr& other) noexcept
    //         : ptr_(other.ptr_), control_(other.control_) {
    //         if (control_) {
    //             control_->weak_count.fetch_add(1, std::memory_order_relaxed);
    //         }
    //     }
    //     any_weak_ptr& operator=(const any_weak_ptr& other) noexcept {
    //         if (this != &other) {
    //             // Decrease current object's weak count
    //             if (control_ && control_->weak_count.fetch_sub(1, std::memory_order_relaxed) == 1 &&
    //                 control_->ref_count.load(std::memory_order_relaxed) == 0) {
    //                 delete control_;
    //             }
    //             // Copy from other
    //             ptr_ = other.ptr_;
    //             control_ = other.control_;
    //             if (control_) {
    //                 control_->weak_count.fetch_add(1, std::memory_order_relaxed);
    //             }
    //         }
    //         return *this;
    //     }

    //     ~any_weak_ptr() {
    //         if (control_ && control_->weak_count.fetch_sub(1, std::memory_order_relaxed) == 1 &&
    //             control_->ref_count.load(std::memory_order_relaxed) == 0) {
    //             delete control_;
    //         }
    //     }

    //     explicit operator bool() const noexcept {
    //         return ptr_ != nullptr;
    //     }

    //     any_shared_ptr lock() const noexcept {
    //         if (control_ && control_->ref_count.load(std::memory_order_relaxed) > 0) {
    //             control_->ref_count.fetch_add(1, std::memory_order_relaxed);
    //             return any_shared_ptr(ptr_, control_);
    //         }
    //         return any_shared_ptr();
    //     }
    // };

    template<typename T, typename... Args>
    std::pair<any_unique_ptr, T*> make_any_unique(Args&&... args) {
        any_unique_ptr aup(new detail::any_derived<T>(std::forward<Args>(args)...));
        T* p = aup.get<T>();
        return std::make_pair(std::move(aup), p);
    }

    template<typename T, typename... Args>
    std::pair<any_shared_ptr, T*> make_any_shared(Args&&... args) {
        any_shared_ptr asp(new detail::any_derived<T>(std::forward<Args>(args)...));
        T* p = asp.get<T>();
        return std::make_pair(std::move(asp), p);
    }
}