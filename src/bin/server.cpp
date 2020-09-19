#include <memory>
#include <iostream>

#include "echo_server_delegate.h"
#include "server_tcp.h"
#include "server_udp.h"

int main(int argc, char **argv) {
    static const uint8_t numRequiredParameters = 2;

    if (argc < numRequiredParameters + 1) {
        std::cout << "Usage: " << argv[0] << " <port> <protocol:TCP|UDP> [connection_queue_size] [operations_timeout_s]\n"
                  << "e.g. " << argv[0] << " 8888 TCP 10 5\n"
                  << "Arguments:\n"
                  << "* port - port to use\n"
                  << "* protocol - protocol to use ('TCP' or 'UDP')\n"
                  << "* connection_queue_size - number of connection requests to be queued before further"
                     " requests are refused, default is 1024 (only when TCP is used)\n"
                  << "* operations_timeout_s - all operations timeout in seconds, default is 5 (only when TCP is used)"
                  << std::endl;
        return 0;
    }

    // 1. Parse port

    uint16_t port;

    try {
        port = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "Invalid port number: " << argv[1] << std::endl;
        return 1;
    }

    // 2. Parse protocol

    std::string protocol(argv[2]);

    if (protocol != "TCP" && protocol != "UDP") {
        std::cerr << "Invalid protocol : " << argv[2] << std::endl;
        return 1;
    }

    // 3. Try to parse connection queue size and timeout

    int connectionQueueSize = 1024;
    long operationsTimoutSeconds = 5;

    if (argc > 3) {
        try {
            connectionQueueSize = std::stoi(argv[3]);
        } catch (...) {
            std::cerr << "Invalid connection queue size: " << argv[3] << std::endl;
            return 1;
        }
    }

    if (argc > 4) {
        try {
            operationsTimoutSeconds = std::stol(argv[4]);
        } catch (...) {
            std::cerr << "Invalid operations timeout: " << argv[4] << std::endl;
            return 1;
        }
    }

    // 4. Create server

    Server *server = nullptr;

    if (protocol == "TCP") {
        server = new ServerTcp(port, std::cout, connectionQueueSize, operationsTimoutSeconds);
    } else if (protocol == "UDP") {
        server = new ServerUdp(port, std::cout);
    } else {
        // Should never be there
        throw;
    }

    // 5. Create echo server delegate

    ServerDelegate *serverDelegate = new EchoServerDelegate;

    // 6. Run event loop

    server->eventLoop(serverDelegate);

    // 7. Deallocate stuff (actually control flow will never be here)

    delete serverDelegate;
    delete server;

    return 0;
}