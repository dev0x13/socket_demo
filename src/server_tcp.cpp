#include <vector>
#include <cstring>
#include <csignal>
#include <cassert>

#include <poll.h>
#include <netinet/in.h>
#include <unistd.h>

#include <socket_demo/defines.h>
#include <socket_demo/server_delegate.h>

#include "server_tcp.h"
#include "utils.h"

// We store it globally since we are not able to make signal handler
// stateful (i.e. class method).
// This is definitely non-production solution, but let it be for this toy code.
// Btw this automatically makes ServerUdp non thread safe.
// Unlike UDP server case we store poll structures for alive connections.
static std::vector<pollfd> descriptors;

void ServerTcp::gracefulShutdown() {
    for (const auto& descriptor: descriptors) {
        shutdown(descriptor.fd, SHUT_RDWR);
        close(descriptor.fd);
    }

    descriptors.erase(descriptors.begin(), descriptors.end());
}

void ServerTcp::signalHandler(int) {
    gracefulShutdown();

    std::exit(0);
}

ServerTcp::ServerTcp(uint16_t port, std::ostream& logStream, int maxNumConnections, long timeoutSeconds)
    : logStream(logStream)
{
    // 0. Init socket address

    sockaddr_in socketAddress{};
    std::memset(&socketAddress, 0, sizeof(socketAddress));

    // 0.1. Set signal handlers

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 1. Create socket

    int listeningSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listeningSocket < 0) {
        throw std::runtime_error("Cannot create TCP socket: " + getError());
    }

    // 2. Set timeout on reading and writing

    if (timeoutSeconds <= 0) {
        throw std::runtime_error("Timeout should be a positive value");
    }

    timeval timeout{ timeoutSeconds, 0 };

    if (setsockopt(listeningSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Cannot set SO_RCVTIMEO: " + getError());
    }

    if (setsockopt(listeningSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Cannot set SO_SNDTIMEO: " + getError());
    }

    // 3. Set socket-reusable option to enable server quick restarts

    {
        int enable = 1;

        if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
            throw std::runtime_error("Cannot set SO_REUSEADDR: " + getError());
        }

        if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0) {
            throw std::runtime_error("Cannot set SO_REUSEADDR: " + getError());
        }
    }

    // 3. Bind and listen TCP socket

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
    socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listeningSocket, reinterpret_cast<sockaddr*>(&socketAddress), sizeof(socketAddress)) < 0) {
        close(listeningSocket);
        throw std::runtime_error("Cannot bind TCP socket: " + getError());
    }

    if (listen(listeningSocket, maxNumConnections) < 0) {
        close(listeningSocket);
        throw std::runtime_error("Cannot listen TCP socket: " + getError());
    }

    // 4. Save listening socket for polling

    descriptors.push_back({ .fd = listeningSocket, .events = POLLIN, .revents = 0 });

    logStream << "Listening on " << port << std::endl;
}

void ServerTcp::eventLoop(ServerDelegate *serverDelegate) {
    char *buffer = new char[MAX_MESSAGE_LENGTH_BYTES];

    for (;;) {
        // 1. Poll stored descriptors until new connection is requested or
        // any connection socket is readable

        if (poll(descriptors.data(), descriptors.size(), -1) < 0) {
            throw std::runtime_error("Socket polling failed!");
        }

        assert(!descriptors.empty());

        // 2. Handle and try to accept new connection on listening socket

        if (descriptors[0].revents & POLLIN) {
            int acceptedFd;

            acceptedFd = accept(descriptors[0].fd, nullptr, nullptr);

            if (acceptedFd < 0) {
                logStream << "Cannot accept connection: " << getError() << std::endl;
            } else {
                logStream << "Accepted connection: " << acceptedFd << std::endl;
                descriptors.push_back({ .fd = acceptedFd, .events = POLLIN, .revents = 0 });
            }
        }

        // 3. Poll other sockets

        for (size_t i = 1; i < descriptors.size(); ++i) {
            if (descriptors[i].revents & POLLIN) {
                std::memset(buffer, 0, MAX_MESSAGE_LENGTH_BYTES);

                // 3.1. Try to read from polled socket

                int numBytesReceived = recv(descriptors[i].fd, buffer, MAX_MESSAGE_LENGTH_BYTES, 0);

                if (numBytesReceived <= 0) {
                    if (numBytesReceived == 0) {
                        // 0 == client disconnected
                        logStream << "Disconnected " << descriptors[i].fd << std::endl;
                    } else {
                        // -1 == reading error
                        logStream << "Cannot read message from " << descriptors[i].fd
                                  << ": " << getError() << std::endl;
                    }

                    // In both cases close this connection

                    close(descriptors[i].fd);
                    descriptors.erase(descriptors.begin() + i);
                    continue;
                }

                // 3.2. If read was successful, process received message

                logStream << "Received message from " << descriptors[i].fd << " [" << numBytesReceived
                          << "]: " << buffer << std::endl;

                std::string response;

                if (serverDelegate) {
                    response = serverDelegate->process(std::string(buffer, numBytesReceived));
                }

                // 3.3. Send response

                if (!response.empty()) {
                    if (response.size() >= MAX_MESSAGE_LENGTH_BYTES) {
                        logStream << "Response is too long, it will be truncated to "
                                  << MAX_MESSAGE_LENGTH_BYTES << " bytes" << std::endl;
                    }

                    const int64_t actualResponseSize = std::min(response.size(), static_cast<size_t>(MAX_MESSAGE_LENGTH_BYTES));

                    if (send(descriptors[i].fd, response.data(), actualResponseSize, 0) != actualResponseSize) {
                        logStream << "Cannot send message" << std::endl;
                    }
                }
            }
        }
    }

    logStream << "Exited the event loop" << std::endl;
    delete[] buffer;
}

ServerTcp::~ServerTcp() {
    ServerTcp::gracefulShutdown();
}