#pragma once

#include <string>

// Socket server delegate interface.
// Needed because we don't want to mix network code with data processing code.
class ServerDelegate {
public:
    virtual std::string process(const std::string& received) noexcept = 0;

    virtual ~ServerDelegate() = default;
};