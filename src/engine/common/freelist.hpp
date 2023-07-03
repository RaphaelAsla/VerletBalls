#pragma once
#include <vector>

template <typename T>
struct FreeList {
    union FreeElement {
        T element;
        int next;
    };

    std::vector<FreeElement> data;
    int first_free;

    FreeList() : first_free{-1} {}

    size_t insert(const T& element) {
        if (first_free != -1) {
            const int index = first_free;
            first_free = data[first_free].next;
            data[index].element = element;
            return index;
        } else {
            FreeElement fe{element};
            data.emplace_back(fe);
        }
        return data.size() - 1;
    }

    void erase(int n) {
        data[n].next = first_free;
        first_free = n;
    }

    void clear() {
        data.clear();
        first_free = -1;
    }

    size_t size() const {
        return data.size();
    }

    T& operator[](int n) noexcept {
        return data[n].element;
    }

    const T& operator[](int n) const noexcept {
        return data[n].element;
    }

    using iterator = typename std::vector<FreeElement>::iterator;
    using const_iterator = typename std::vector<FreeElement>::const_iterator;

    iterator begin() {
        return data.begin();
    }

    iterator end() {
        return data.end();
    }

    const_iterator begin() const {
        return data.begin();
    }

    const_iterator end() const {
        return data.end();
    }

    const_iterator cbegin() const {
        return data.cbegin();
    }

    const_iterator cend() const {
        return data.cend();
    }
};
