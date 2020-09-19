#pragma once

#include <cstdint>

class ServerDelegate;

// Socker server interface
class Server {
public:
    virtual void eventLoop(ServerDelegate *serverDelegate = nullptr) = 0;

    virtual ~Server() = default;
};