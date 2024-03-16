#pragma once
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "array_ptr.h"

struct ReserveProxyObj {
    size_t capacity;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj{capacity_to_reserve};
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;
    using ItemsPtr = ArrayPtr<Type>;

    SimpleVector() noexcept = default;
    
    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type{}) 
    {
    }

    SimpleVector(size_t size, const Type& value)
        : items_(size)
        , size_(size)
        , capacity_(size) {
        std::fill(items_.Get(), items_.Get() + size_, value);
    }

    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size())
        , size_(init.size())
        , capacity_(init.size()) {
        std::copy(init.begin(), init.end(), items_.Get());
    }

    SimpleVector(const SimpleVector& other)
        : items_(other.size_)
        , size_(other.size_)
        , capacity_(other.size_) {
        std::copy(other.items_.Get(), other.items_.Get() + size_, items_.Get());
    }

    SimpleVector(ReserveProxyObj reserved)
        : items_(reserved.capacity)
        , size_(0)
        , capacity_(reserved.capacity) 
    {
    }

    SimpleVector(SimpleVector&& other) noexcept
        : items_(std::move(other.items_))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0)) 
    {
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        if (index >= size_) {
            throw std::out_of_range(std::string("Item index is out of range"));
        }
        return items_[index];
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&rhs != this) {
            if (rhs.IsEmpty()) {
                Clear(); // Оптимизация для случая присваивания пустого вектора
            } else {
                SimpleVector rhs_copy(rhs);  // Copy-and-swap
                swap(rhs_copy);
            }
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& other) {
        items_ = std::move(other.items_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);

        return *this;
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range(std::string("Item index is out of range"));
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range(std::string("Item index is out of range"));
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            auto new_items = ReallocateMove(new_capacity);
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }

    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            const size_t new_capacity = std::max(capacity_ * 2, new_size);  // Вычисляем вместимость вектора
            auto new_items = ReallocateMove(new_capacity);
            FillWithDefaultValue(&new_items[size_], &new_items[new_size]);
            items_.swap(new_items);
            capacity_ = new_capacity;
        } else if (new_size > size_) {
            assert(new_size <= capacity_);
            FillWithDefaultValue(&items_[size_], &items_[new_size]);
        }
        size_ = new_size;
    }

    void PushBack(const Type& item) {
        const size_t new_size = size_ + 1;
        if (new_size > capacity_) {
            const size_t new_capacity = std::max(capacity_ * 2, new_size);  // Вычисляем вместимость вектора
            auto new_items = ReallocateMove(new_capacity); 
            new_items.Get()[size_] = item;                 
            capacity_ = new_capacity;
            items_.swap(new_items);
        } else {
            items_.Get()[size_] = item;
        }
        size_ = new_size;
    }

    void PushBack(Type&& item) {
        const size_t new_size = size_ + 1;
        if (new_size > capacity_) {
            const size_t new_capacity = std::max(capacity_ * 2, new_size);  // Вычисляем вместимость вектора

            auto new_items = ReallocateMove(new_capacity);
            new_items.Get()[size_] = std::move(item);

            capacity_ = new_capacity;
            items_.swap(new_items);
        } else {
            items_.Get()[size_] = std::move(item);
        }
        size_ = new_size;
    }

    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(begin() <= pos && pos <= end());

        size_t new_size = size_ + 1;
        size_t new_item_offset = pos - cbegin();
        if (new_size <= capacity_) {
            Iterator mutable_pos = begin() + new_item_offset;
            std::copy_backward(pos, cend(), end() + 1);
            *mutable_pos = value;           
        } else {                            
            size_t new_capacity = std::max(capacity_ * 2, new_size);  // Вычисляем вместимость вектора
            ItemsPtr new_items(new_capacity); 
            Iterator new_items_pos = new_items.Get() + new_item_offset;
            // Копируем элементы, предшествующие вставляемому
            std::copy(cbegin(), pos, new_items.Get()); 
            *new_items_pos = value; 
            // Копируем элементы следующие за вставляемым
            std::copy(pos, cend(), new_items_pos + 1);
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        size_ = new_size;
        return begin() + new_item_offset;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(begin() <= pos && pos <= end());

        size_t new_size = size_ + 1;
        size_t new_item_offset = pos - cbegin();
        Iterator mutable_pos = begin() + new_item_offset;
        if (new_size <= capacity_) {
            std::move_backward(mutable_pos, end(), end() + 1);
            *mutable_pos = std::move(value);
        } else {
            size_t new_capacity = std::max(capacity_ * 2, new_size); // Вычисляем вместимость вектора
            ItemsPtr new_items(new_capacity);
            Iterator new_items_pos = new_items.Get() + new_item_offset;
            std::move(begin(), mutable_pos, new_items.Get());
            *new_items_pos = std::move(value);
            std::move(mutable_pos, end(), new_items_pos + 1);
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        size_ = new_size;
        return begin() + new_item_offset;
    }

    Iterator Erase(ConstIterator pos) {
        assert(begin() <= pos && pos < end());

        Iterator mutable_pos = begin() + (pos - cbegin());
        std::move(mutable_pos + 1, end(), mutable_pos); // Переносим "хвост" влево на места удаляемого элемента
        --size_;

        return mutable_pos;
    }

    Iterator begin() noexcept {
        return items_.Get();
    }

    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    ItemsPtr ReallocateMove(size_t new_capacity) const {
        ItemsPtr new_items(new_capacity);
        size_t copy_size = std::min(new_capacity, size_);
        std::move(items_.Get(), items_.Get() + copy_size, new_items.Get());
        return ItemsPtr(new_items.Release());
    }

    static void FillWithDefaultValue(Type* from, Type* to) {
        while (from != to) {
            *from = Type{};
            ++from;
        }
    }
    
    ItemsPtr items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs.GetSize() == rhs.GetSize()) && std::equal(lhs.begin(), lhs.end(), rhs.begin());  
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);  
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs <= lhs;
}