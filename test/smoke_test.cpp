#include <memory>
#include <iostream>
#include <random>
#include <cstring>
#include <algorithm>
#include <thread>

#include <socket_demo/defines.h>

#include "client_udp.h"
#include "client_tcp.h"
#include "echo_server_delegate.h"

// Generates random string containing numbers and letters separated by spaces
std::string generateRandomString(){
    static constexpr auto chars =
        "0123456789"
        "a"
        " -";

    // `thread_local` because we call this from multiple threads
    thread_local std::random_device rng;

    // Separate distributions for characters and message length
    std::uniform_int_distribution<size_t> charDist(0, std::strlen(chars) - 1);
    std::uniform_int_distribution<size_t> lengthDist(0, MAX_MESSAGE_LENGTH_BYTES);

    const std::size_t length = lengthDist(rng);

    std::string result(length, '\0');
    std::generate_n(begin(result), length, [&]() { return chars[charDist(rng)]; });

    return result;
}

int main(int argc, char **argv) {
    static const uint8_t numRequiredParameters = 3;

    if (argc < numRequiredParameters + 1) {
        std::cout << "Usage: " << argv[0]
                  << " <server_address> <port> <protocol:TCP|UDP> [operations_timeout_s] [num_connections]\n"
                  << "e.g. " << argv[0] << " 127.0.0.1 8888 TCP 5 1024\n"
                  << "Arguments:\n"
                  << "* server_address - server address\n"
                  << "* port - port to use\n"
                  << "* protocol - protocol to use ('TCP' or 'UDP')\n"
                  << "* operations_timeout_s - all operations timeout in seconds, default is 5\n"
                  << "* num_connections - number of simultaneous connections (threads), default is 512\n"
                  << "* udp_max_tries - send-receive attempts to make until success (only when UDP is used), default is 10"
                  << std::endl;
        return 0;
    }

    // 1. Parse server address

    std::string serverAddress = argv[1];

    // 2. Parse server port

    uint16_t port;

    try {
        port = std::stoi(argv[2]);
    } catch (...) {
        std::cerr << "Invalid port number: " << argv[1] << std::endl;
        return 1;
    }

    // 3. Parse protocol

    std::string protocol(argv[3]);

    if (protocol != "TCP" && protocol != "UDP") {
        std::cerr << "Invalid protocol : " << argv[2] << std::endl;
        return 1;
    }

    // 4. Try to parse timeout

    long operationsTimoutSeconds = 5;

    if (argc > 4) {
        try {
            operationsTimoutSeconds = std::stol(argv[4]);
        } catch (...) {
            std::cerr << "Invalid operations timeout: " << argv[4] << std::endl;
            return 1;
        }
    }

    // 4. Try to parse connections number

    size_t numConnections = 512;

    if (argc > 5) {
        try {
            numConnections = std::stoull(argv[5]);
        } catch (...) {
            std::cerr << "Invalid number of connections: " << argv[5] << std::endl;
            return 1;
        }
    }

    // 5. Try to parse UDP max tries number

    size_t udpMaxTries = 10;

    if (argc > 6) {
        try {
            udpMaxTries = std::stoull(argv[6]);
        } catch (...) {
            std::cerr << "Invalid UDP max tries number: " << argv[6] << std::endl;
            return 1;
        }
    }

    // 6. Create echo server delegate. We need this since we need to validate
    // whether server returned correct output

    EchoServerDelegate echoProcessor;

    // 7. Spawn multiple client threads. Each thread generates random string,
    // send it to server, validates sending and receiving procedures and validates
    // output

    std::vector<std::thread> threads;
    threads.reserve(numConnections);

    for (size_t i = 0; i < numConnections; ++i) {
        threads.emplace_back([&] {
            std::unique_ptr<Client> client;

            if (protocol == "TCP") {
                client.reset(new ClientTcp(serverAddress, port, std::cout, operationsTimoutSeconds));
            } else if (protocol == "UDP") {
                client.reset(new ClientUdp(serverAddress, port, std::cout, operationsTimoutSeconds));
            } else {
                // Should never be there
                throw;
            }

            const std::string msg = generateRandomString();

            if (!client->send(msg)) {
                throw std::runtime_error("Cannot send message");
            }

            std::string response;

            if (protocol == "TCP") {
                // Pretty simple for TCP

                if (!client->receive(response)) {
                    throw std::runtime_error("Cannot receive message");
                }
            } else if (protocol == "UDP") {
                // This is selective-repeat-like ARQ protocol, but each message fits in a single
                // datagram, so we basically send the same message multiple times

                bool receivedResponse = false;

                for (size_t i = 0; i < udpMaxTries; ++i) {
                    if (client->receive(response)) {
                        receivedResponse = true;
                        break;
                    }

                    client->send(msg);
                }

                if (!receivedResponse) {
                    // At least we've tried
                    throw std::runtime_error("Cannot receive message");
                }
            } else {
                // Should never be there
                throw;
            }

            if (response != echoProcessor.process(msg)) {
                throw std::runtime_error("Invalid response");
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "OK!" << std::endl;

    return 0;
}