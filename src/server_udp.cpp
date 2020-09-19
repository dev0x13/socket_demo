#include <socket_demo/defines.h>
#include <socket_demo/server_delegate.h>

#include <vector>
#include <cstring>
#include <csignal>

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server_udp.h"
#include "utils.h"

// We store it globally since we are not able to make signal handler
// stateful (i.e. class method).
// This is definitely non-production solution, but let it be for this toy code.
// Btw this automatically makes ServerUdp non thread safe.
int udpSocketDescriptor;

void ServerUdp::gracefulShutdown() {
    shutdown(udpSocketDescriptor, SHUT_RDWR);
    close(udpSocketDescriptor);
}

void ServerUdp::signalHandler(int) {
    gracefulShutdown();
    std::exit(0);
}

ServerUdp::ServerUdp(uint16_t port, std::ostream& logStream) : logStream(logStream)
{
    // 0. Init socket address

    sockaddr_in socketAddress{};
    std::memset(&socketAddress, 0, sizeof(socketAddress));

    // 0.1. Set up signal handlers

    signal(SIGINT, ServerUdp::signalHandler);
    signal(SIGTERM, ServerUdp::signalHandler);

    // 1. Create UDP socket

    udpSocketDescriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (udpSocketDescriptor == -1) {
        throw std::runtime_error("Cannot create UDP socket: " + getError());
    }

    // 2. Set up socket options. This is done mostly to enable server quick restarts

    {
        int enable = 1;

        if (setsockopt(udpSocketDescriptor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
            throw std::runtime_error("Cannot set SO_REUSEADDR: " + getError());
        }

        if (setsockopt(udpSocketDescriptor, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0) {
            throw std::runtime_error("Cannot set SO_REUSEADDR: " + getError());
        }
    }

    // 3. Bind UDP socket

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
    socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udpSocketDescriptor, reinterpret_cast<sockaddr*>(&socketAddress), sizeof(socketAddress)) == -1) {
        close(udpSocketDescriptor);
        throw std::runtime_error("Cannot bind UDP socket: " + getError());
    }

    logStream << "Listening on " << port << std::endl;
}

void ServerUdp::eventLoop(ServerDelegate *serverDelegate) {
    char *buffer = new char[MAX_MESSAGE_LENGTH_BYTES];

    for (;;) {
        // 0. Create client address

        socklen_t addressLength;
        sockaddr_in clientAddress{};

        std::memset(&clientAddress, 0, sizeof(sockaddr_in));
        addressLength = sizeof(clientAddress);

        // 1. Read message

        std::memset(buffer, 0, MAX_MESSAGE_LENGTH_BYTES);
        int numBytesReceived = recvfrom(udpSocketDescriptor, buffer, MAX_MESSAGE_LENGTH_BYTES,
                                        0, reinterpret_cast<sockaddr*>(&clientAddress), &addressLength);

        if (numBytesReceived < 0) {
            logStream << "Cannot read message: " << getError() << std::endl;
        } else if (numBytesReceived == 0) {
            logStream << "Message is empty" << std::endl;
        } else {
            // 2. Get client address

            char from[INET_ADDRSTRLEN + 1];
            from[0] = '\0';

            if (inet_ntop(AF_INET, &clientAddress.sin_addr, from, INET_ADDRSTRLEN) != nullptr) {
                logStream << "Received message from " << from << ":" << ntohs(clientAddress.sin_port)
                          << " [" << numBytesReceived << "] : " << buffer << std::endl;
            } else {
                logStream << "Cannot convert address: " << getError() << std::endl;
                logStream << "Received message from unknown address " << "[" << numBytesReceived
                          << "] : " << buffer << std::endl;
            }

            // 3. Process message

            std::string response;

            if (serverDelegate) {
                response = serverDelegate->process(std::string(buffer, numBytesReceived));
            }

            if (!response.empty()) {
                if (response.size() >= MAX_MESSAGE_LENGTH_BYTES) {
                    logStream << "Response is too long, it will be truncated to "
                              << MAX_MESSAGE_LENGTH_BYTES << " bytes" << std::endl;
                }

                const int64_t actualResponseSize = std::min(response.size(), static_cast<size_t>(MAX_MESSAGE_LENGTH_BYTES));

                if (sendto(udpSocketDescriptor, response.data(), actualResponseSize,
                           MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&clientAddress),
                           sizeof(clientAddress)) != actualResponseSize)
                {
                    logStream << "Cannot send message: " << getError() << std::endl;
                }
            }
        }
    }

    logStream << "Exited the event loop" << std::endl;
    delete[] buffer;
}

ServerUdp::~ServerUdp() {
    ServerUdp::gracefulShutdown();
}