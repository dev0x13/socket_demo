#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using Number = uint64_t;

struct EchoResult {
    Number sum = 0;
    std::vector<Number> numbers;

    void accumulate(Number number) {
        numbers.push_back(number);
        sum += number;
    }

    bool empty() const {
        return numbers.empty();
    }

    std::string toMessage() {
        if (empty()) {
            return "";
        }

        std::sort(numbers.begin(), numbers.end());

        std::stringstream ss;

        const size_t numNumbers = numbers.size();

        ss << numbers[0];

        for (size_t i = 0; i < numNumbers; ++i) {
            ss << " " << numbers[1];
        }

        ss << '\n';

        ss << sum;

        return ss.str();
    }
};

class EchoProcessor {
public:
    static std::string echo(const std::string& message) {
        std::stringstream ss(message);

        Number tmp;

        EchoResult echoResult;

        while (!ss.eof()) {

            if (ss >> tmp) {
                echoResult.accumulate(tmp);
            }
        }

        return echoResult.empty() ? message : echoResult.toMessage();
    }
};

class ClientSocket {
public:
    virtual bool send(const std::string& message) = 0;

    virtual bool receive(std::string& message) = 0;

    virtual ~ClientSocket() = default;
};

class ServerSocket {
public:
    virtual bool send(const std::string& message) = 0;

    virtual bool receive(std::string& message) = 0;

    virtual ~ServerSocket() = default;
};

class ServerSocketTCP: public ServerSocket {
public:
    explicit ServerSocketTCP(uint16_t port, int maxNumConnections = 10, long timeoutSeconds = 5) {

        // 1. Create socket

        socketDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (socketDescriptor == -1) {
            throw std::runtime_error("Cannot create TCP socket!");
        }

        // 2. Set timeout on reading and writing

        if (timeoutSeconds <= 0) {
            throw std::runtime_error("Timeout should be a positive value!");
        }

        timeval timeout{ timeoutSeconds, 0 };

        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Cannot set READ timeout!");
        }

        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Cannot set SEND timeout!");
        }

        // 3. Bind and listen

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(socketDescriptor,reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == -1) {
            close(socketDescriptor);
            throw std::runtime_error("Cannot bind TCP socket!");
        }

        if (listen(socketDescriptor, maxNumConnections) == -1) {
            close(socketDescriptor);
            throw std::runtime_error("Cannot listen TCP socket!");
        }

        std::cout << "Listening on " << port << std::endl;
    }

    bool send(const std::string& message) override {
        return static_cast<size_t>(::send(socketDescriptor, message.data(), message.size(), 0)) == message.size();
    }

    bool receive(std::string& message) override {
        int connectionDescriptor = accept(socketDescriptor, nullptr, nullptr);

        if (connectionDescriptor < 0) {
            close(connectionDescriptor);
            return false;
        }

        std::cout << "Connected" << std::endl;

        uint64_t messageSize = 0;

        std::cout << ::recv(socketDescriptor, &messageSize, sizeof(messageSize), 0) << std::endl;

        if (::recv(socketDescriptor, &messageSize, sizeof(messageSize), 0) != sizeof(messageSize)) {
            return false;
        }

        char *buffer = new char[messageSize];

        if (static_cast<uint64_t>(::recv(socketDescriptor, buffer, messageSize, 0)) != messageSize) {
            return false;
        }

        message.assign(buffer, messageSize);

        send(EchoProcessor::echo(message));

        if (shutdown(connectionDescriptor, SHUT_RDWR) == -1) {
            close(connectionDescriptor);
        }

        return true;
    }

    ~ServerSocketTCP() override {
        shutdown(socketDescriptor, SHUT_RDWR);
        close(socketDescriptor);
    };

private:
    sockaddr_in sa{};
    int socketDescriptor;
};

class ClientSocketTCP: public ClientSocket {
public:
    ClientSocketTCP(const std::string& address, uint16_t port, long timeoutSeconds = 5) {

        // 1. Create socket

        socketDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (socketDescriptor == -1) {
            throw std::runtime_error("Cannot create TCP socket!");
        }

        // 2. Set timeout on reading and writing

        if (timeoutSeconds <= 0) {
            throw std::runtime_error("Timeout should be a positive value!");
        }

        timeval timeout{ timeoutSeconds, 0 };

        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Cannot set READ timeout!");
        }

        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Cannot set SEND timeout!");
        }

        // 3. Establish connection

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);

        if (!inet_pton(AF_INET, address.c_str(), &sa.sin_addr)) {
            throw std::invalid_argument("Invalid socket address: " + address);
        }

        if (connect(socketDescriptor, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == -1) {
            close(socketDescriptor);
            throw std::invalid_argument("Connection failed!");
        }
    }

    bool send(const std::string& message) override {
        return static_cast<size_t>(::send(socketDescriptor, message.data(), message.size(), 0)) == message.size();
    }

    bool receive(std::string& message) override {
        uint64_t messageSize = 0;

        if (::read(socketDescriptor, &messageSize, sizeof(messageSize)) != sizeof(messageSize)) {
            return false;
        }

        char *buffer = new char[messageSize];

        if (static_cast<uint64_t>(::read(socketDescriptor, buffer, messageSize)) != messageSize) {
            return false;
        }

        message.assign(buffer, messageSize);

        return true;
    }

    ~ClientSocketTCP() override {
        shutdown(socketDescriptor, SHUT_RDWR);
        close(socketDescriptor);
    };

private:
    sockaddr_in sa{};
    int socketDescriptor;
};