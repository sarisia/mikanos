#pragma once

#include <cstddef>
#include <cstdint>

template <typename T>
struct ArrayLength {};

template <typename T, size_t N>
struct ArrayLength<T[N]> {
    static const size_t value = N;
};

template <typename T>
class MemMapRegister {
private:
    volatile T value_;
    static const size_t len_ = ArrayLength<decltype(T::data)>::value;

public:
    T Read() const {
        T temp;
        for (size_t i = 0; i < len_; ++i) {
            temp.data[i] = value_.data[i];
        }

        return temp;
    }

    void Write(const T& value) {
        for (size_t i = 0; i < len_; ++i) {
            value_.data[i] = value.data[i];
        }
    }
};

template <typename T>
struct DefaultBitmap {
    T data[1];

    DefaultBitmap& operator =(const T& value) {
        data[0] = value;
    }

    operator T() const {
        return data[0];
    }
};

template <typename T>
class ArrayWrapper {
public:
    using ValueType = T;
    using Iterator = ValueType*;
    using ConstIterator = const ValueType*;

    ArrayWrapper(uintptr_t array_base_addr, size_t size)
        : array_(reinterpret_cast<ValueType*>(array_base_addr)),
          size_(size) {}

    size_t Size() const {
        return size_;
    }

    Iterator begin() {
        return array_;
    }

    Iterator end() {
        return array_ + size_;
    }

    ConstIterator cbegin() const {
        return array_;
    }

    ConstIterator cend() const {
        return array_ + size_;
    }

    ValueType& operator [](size_t index) {
        return array_[index];
    }

private:
    ValueType* const array_;
    const size_t size_;
};
