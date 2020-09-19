#include <ostream>
#include <cstring>

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <socket_demo/defines.h>
#include <poll.h>

#include "utils.h"
#include "client_tcp.h"

ClientTcp::ClientTcp(const std::string& address, uint16_t port, std::ostream& logStream, long timeoutSeconds)
    : socketAddress(new sockaddr_in), logStream(logStream)
{
    std::memset(socketAddress, 0, sizeof(sockaddr_in));

    // 1. Create TCP socket

    socketDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socketDescriptor == -1) {
        throw std::runtime_error("Cannot create TCP socket: " + getError());
    }

    // 2. Set timeout on reading and writing

    if (timeoutSeconds <= 0) {
        throw std::runtime_error("Timeout should be a positive value!");
    }

    timeval timeout{ timeoutSeconds, 0 };

    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Cannot set READ timeout: " + getError());
    }

    if (setsockopt(socketDescriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Cannot set SEND timeout: " + getError());
    }

    // 3. Establish connection

    socketAddress->sin_family = AF_INET;
    socketAddress->sin_port = htons(port);

    if (!inet_pton(AF_INET, address.c_str(), &socketAddress->sin_addr)) {
        throw std::invalid_argument("Invalid socket address: " + address);
    }

    if (connect(socketDescriptor, reinterpret_cast<sockaddr*>(socketAddress), sizeof(*socketAddress)) == -1) {
        if (errno == EINPROGRESS) {
            // Connect may act asynchronously, so poll this socket till it ready for writing
            pollfd fd{ .fd = socketDescriptor, .events = POLLOUT, .revents = 0 };
            poll(&fd, 1, -1);
        } else {
            close(socketDescriptor);
            throw std::invalid_argument("Connection failed: " + getError());
        }
    }
}

bool ClientTcp::send(const std::string& data) {
    if (data.empty()) {
        return false;
    }

    if (data.size() >= MAX_MESSAGE_LENGTH_BYTES) {
        logStream << "Data is too long, it will be truncated to "
                  << MAX_MESSAGE_LENGTH_BYTES << " bytes" << std::endl;
    }

    const int64_t actualDataSize = std::min(data.size(), static_cast<size_t>(MAX_MESSAGE_LENGTH_BYTES));

    return ::send(socketDescriptor, data.data(), actualDataSize, 0) == actualDataSize;
}

bool ClientTcp::receive(std::string& message) {
    char *buffer = new char[MAX_MESSAGE_LENGTH_BYTES];

    int numBytesReceived = ::read(socketDescriptor, buffer, MAX_MESSAGE_LENGTH_BYTES);

    if (numBytesReceived <= 0) {
        if (errno == EAGAIN) {
            // Read may act asynchronously, so poll this socket till it ready for reading and
            // then try to read again
            pollfd fd{ .fd = socketDescriptor, .events = POLLIN, .revents = 0 };
            poll(&fd, 1, -1);
            numBytesReceived = ::read(socketDescriptor, buffer, MAX_MESSAGE_LENGTH_BYTES);
        }
    }

    if (numBytesReceived <= 0) {
        logStream << "Cannot receive message: " + getError() << std::endl;
        delete[] buffer;
        return false;
    }

    message.assign(buffer, numBytesReceived);

    delete[] buffer;

    return true;
}

ClientTcp::~ClientTcp() {
    shutdown(socketDescriptor, SHUT_RDWR);
    close(socketDescriptor);
};