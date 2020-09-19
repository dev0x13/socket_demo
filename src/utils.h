#pragma once

#include <string>
#include <cstring>

// Transforms errno to std::string
inline std::string getError() {
    return std::string(std::strerror(errno));
}