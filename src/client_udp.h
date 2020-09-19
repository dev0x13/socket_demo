#pragma once

#include <socket_demo/client.h>

struct sockaddr_in;

// UDP client implementation
class ClientUdp : public Client {
public:
    ClientUdp(const std::string& address, uint16_t port, std::ostream& logStream, long timeoutSeconds = 5);

    bool send(const std::string& data) override;

    bool receive(std::string& data) override;

    ~ClientUdp() override;

    // Forbid copying

    ClientUdp(ClientUdp&) = delete;
    ClientUdp operator=(ClientUdp&) = delete;

private:
    sockaddr_in *serverAddress;
    int socketDescriptor;
    std::ostream& logStream;
};