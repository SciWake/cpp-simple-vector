#pragma once

#include <utility>
#include <cstddef>

template <typename Type>
class ArrayPtr {
public:
    ArrayPtr() noexcept = default;
    explicit ArrayPtr(size_t size) : _rawPtr(size != 0 ? new Type[size]{} : nullptr) {}
    explicit ArrayPtr(Type* raw_ptr) noexcept : _rawPtr(raw_ptr) {}

    ArrayPtr(const ArrayPtr&) = delete;
    ArrayPtr& operator=(const ArrayPtr&) = delete;

    ArrayPtr(ArrayPtr&& other) noexcept : _rawPtr(std::exchange(other._rawPtr, nullptr)) {}
    ArrayPtr& operator=(ArrayPtr&& other) noexcept {
        std::swap(_rawPtr, other._rawPtr);
        return *this;
    }

    ~ArrayPtr() {
        delete[] _rawPtr;
    }

    Type* Release() noexcept {
        return std::exchange(_rawPtr, nullptr);
    }

    Type& operator[](size_t index) noexcept {
        return _rawPtr[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return _rawPtr[index];
    }

    explicit operator bool() const noexcept {
        return _rawPtr != nullptr;
    }

    Type* Get() const noexcept {
        return _rawPtr;
    }

    void swap(ArrayPtr& other) noexcept {
        std::swap(_rawPtr, other._rawPtr);
    }

private:
    Type* _rawPtr = nullptr;
};
