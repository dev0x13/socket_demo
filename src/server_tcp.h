#pragma once

#include <ostream>

#include <socket_demo/server.h>

struct sockaddr_in;

// TCP server implementation
class ServerTcp: public Server {
public:
    ServerTcp(uint16_t port, std::ostream& logStream, int maxNumConnections = 10, long timeoutSeconds = 5);

    void eventLoop(ServerDelegate *serverDelegate = nullptr) override;

    ~ServerTcp() override;

    // Forbid copying

    ServerTcp(ServerTcp&) = delete;
    ServerTcp operator=(ServerTcp&) = delete;

private:
    // Shuts down and closes opened sockets. I am not sure if OS does not take care
    // of it on process termination, so let it be
    static void gracefulShutdown();

    // Needed to invoke `gracefulShutdown` on SIGINT and SIGTERM
    static void signalHandler(int);

    std::ostream& logStream;
};