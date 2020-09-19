#include <ostream>
#include <cstring>

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <socket_demo/defines.h>

#include "utils.h"
#include "client_udp.h"

ClientUdp::ClientUdp(const std::string& address, uint16_t port, std::ostream& logStream, long timeoutSeconds)
    : serverAddress(new sockaddr_in), logStream(logStream)
{
    std::memset(serverAddress, 0, sizeof(sockaddr_in));

    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(port);

    if (!inet_pton(AF_INET, address.c_str(), &serverAddress->sin_addr)) {
        throw std::invalid_argument("Invalid socket address: " + address);
    }

    // 1. Create UDP socket

    socketDescriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketDescriptor == -1) {
        throw std::runtime_error("Cannot create TCP socket: " + getError());
    }

    // 2. Set timeout on reading (writing is async))

    if (timeoutSeconds <= 0) {
        throw std::runtime_error("Timeout should be a positive value!");
    }

    timeval timeout{ timeoutSeconds, 0 };

    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Cannot set READ timeout: " + getError());
    }
}

bool ClientUdp::send(const std::string& data) {
    if (data.empty()) {
        return false;
    }

    if (data.size() >= MAX_MESSAGE_LENGTH_BYTES) {
        logStream << "Data is too long, it will be truncated to "
                  << MAX_MESSAGE_LENGTH_BYTES << " bytes" << std::endl;
    }

    const int64_t actualDataSize = std::min(data.size(), static_cast<size_t>(MAX_MESSAGE_LENGTH_BYTES));

    return sendto(socketDescriptor, data.data(), actualDataSize, MSG_DONTWAIT,
                  reinterpret_cast<sockaddr*>(serverAddress), sizeof(*serverAddress)) == actualDataSize;
}

bool ClientUdp::receive(std::string& message) {
    char *buffer = new char[MAX_MESSAGE_LENGTH_BYTES];

    int numBytesReceived = recv(socketDescriptor, buffer, MAX_MESSAGE_LENGTH_BYTES, 0);

    if (numBytesReceived <= 0) {
        // Don't even report anything because failed receiving is common for UDP
        delete[] buffer;
        return false;
    }

    message.assign(buffer, numBytesReceived);

    delete[] buffer;

    return true;
}

ClientUdp::~ClientUdp() {
    delete serverAddress;
    shutdown(socketDescriptor, SHUT_RDWR);
    close(socketDescriptor);
};