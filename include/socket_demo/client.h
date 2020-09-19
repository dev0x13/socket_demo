#pragma once

#include <string>

// Socket client interface
class Client {
public:
    virtual bool send(const std::string& data) = 0;

    virtual bool receive(std::string& data) = 0;

    virtual ~Client() = default;
};