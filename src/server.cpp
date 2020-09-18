#include <memory>
#include <iostream>

#include "socket.h"

int main() {
    std::unique_ptr<ServerSocket> socket(new ServerSocketTCP(8888));

    std::string msg;

    for (;;) {
        socket->receive(msg);
    }

    return 0;
}