#include <iostream>

#include "client_udp.h"
#include "client_tcp.h"

int main(int argc, char **argv) {
    static const uint8_t numRequiredParameters = 3;

    if (argc < numRequiredParameters + 1) {
        std::cout << "Usage: " << argv[0]
                  << " <server_address> <port> <protocol:TCP|UDP> [operations_timeout_s]\n"
                  << "e.g. " << argv[0] << " 127.0.0.1 8888 TCP 5\n"
                  << "Arguments:\n"
                  << "* server_address - server address\n"
                  << "* port - port to use\n"
                  << "* protocol - protocol to use ('TCP' or 'UDP')\n"
                  << "* operations_timeout_s - all operations timeout in seconds, default is 5\n"
                  << "* udp_max_tries - number of send-receive attempts to make (only when UDP is used), default is 10"
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

    // 5. Try to parse UDP max tries number

    size_t udpMaxTries = 10;

    if (argc > 5) {
        try {
            udpMaxTries = std::stoull(argv[5]);
        } catch (...) {
            std::cerr << "Invalid UDP max tries number: " << argv[5] << std::endl;
            return 1;
        }
    }

    // 6. Create client

    Client *client = nullptr;

    if (protocol == "TCP") {
        client = new ClientTcp(serverAddress, port, std::cout, operationsTimoutSeconds);
    } else if (protocol == "UDP") {
        client = new ClientUdp(serverAddress, port, std::cout, operationsTimoutSeconds);
    } else {
        // Should never be there
        throw;
    }

    // 7. Start input-send loop

    std::string msg;

    for (;;) {
        std::cout << "Enter your message: ";
        std::getline(std::cin, msg);

        if (msg.length() != 0) {
            client->send(msg);

            if (protocol == "TCP") {
                // TCP is simple one

                if (client->receive(msg)) {
                    std::cout << msg << std::endl;
                } else {
                    std::cout << "Error receiving response from server" << std::endl;
                }
            } else {
                // Try several times and then give up

                bool isReceived = false;

                for (size_t i = 0; i < udpMaxTries; ++i) {
                    if (client->receive(msg)) {
                        isReceived = true;
                        break;
                    }

                    client->send(msg);
                }

                if (isReceived) {
                    std::cout << msg << std::endl;
                } else {
                    std::cout << "Error receiving response from server" << std::endl;
                }
            }
        }
    }

    delete client;

    return 0;
}