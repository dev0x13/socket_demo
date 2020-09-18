#include <memory>
#include <iostream>

#include "socket.h"

int main() {
    std::unique_ptr<ClientSocket> socket(new ClientSocketTCP("127.0.0.1", 8888));

    std::string msg;

    for (;;) {
        std::cout << "Enter your message: ";
        std::getline(std::cin, msg);

        socket->send(msg);

        if (socket->receive(msg)) {
            std::cout << msg << std::endl;
        } else {
            std::cout << "Error receiving response from server" << std::endl;
        }
    }

    return 0;
}