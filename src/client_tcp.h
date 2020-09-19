#pragma once

#include <socket_demo/client.h>

struct sockaddr_in;

// TCP client implementation
class ClientTcp : public Client {
public:
    ClientTcp(const std::string& address, uint16_t port, std::ostream& logStream, long timeoutSeconds = 5);

    bool send(const std::string& data) override;

    bool receive(std::string& data) override;

    ~ClientTcp() override;

    // Forbid copying

    ClientTcp(ClientTcp&) = delete;
    ClientTcp operator=(ClientTcp&) = delete;

private:
    sockaddr_in *socketAddress;
    int socketDescriptor;
    std::ostream& logStream;
};