#pragma once

#include <ostream>

#include <socket_demo/server.h>

struct sockaddr_in;

// UDP server implementation
class ServerUdp: public Server {
public:
    ServerUdp(uint16_t port, std::ostream& logStream);

    void eventLoop(ServerDelegate *serverDelegate = nullptr) override;

    ~ServerUdp() override;

    // Forbid copying

    ServerUdp(ServerUdp&) = delete;
    ServerUdp operator=(ServerUdp&) = delete;

private:
    // Shuts down and closes opened sockets. I am not sure if OS does not take care
    // of it on process termination, so let it be
    static void gracefulShutdown();

    // Needed to invoke `gracefulShutdown` on SIGINT and SIGTERM
    static void signalHandler(int);

    std::ostream& logStream;
};