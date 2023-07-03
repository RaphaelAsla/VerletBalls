#pragma once
#include <sstream>

template <typename T>
std::string toString(T value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}
