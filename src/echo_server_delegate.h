#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <sstream>

#include <socket_demo/server_delegate.h>

using Number = int64_t;

// Auxiliary class for constructing echo messages
struct EchoResult {
    void accumulate(Number number);

    bool empty() const;

    std::string toMessage();

private:
    Number sum = 0;
    std::vector<Number> numbers;
};

class EchoServerDelegate: public ServerDelegate {
public:
    std::string process(const std::string& message) noexcept override;
};