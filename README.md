# C++ test task using Berkeley Sockets API

This repository hosts the implementation of C++ test task. Build is performed using CMake.

## Problem statement

Need to develop two Linux applications: a client and a server.

Client sends a message entered by user, and server processes this message once received: looks for all integer numbers in the message, calculates the sum and sends the result back.
If the message does not contain any numbers, server just echoes the source message. It can be assumed that default types can store all integer numbers present in message.

Assuming client sent the following message:
```
20 apples, 30 bananas, 15 peaches and 1 watermelon
```
They should receive the following response:
```
1 15 20 30
66
```

**Mandatory requirements:**

* C++ or C should be used for implementation (STL library usage is welcome, others are not).
* Network communication should be implemented using Berkeley Sockets API (other third-party libraries and frameworks are not allowed).
* Both client and server should support the following protocols: TCP, UDP.
* Server should be able to correctly process multiple concurrent connection for both protocols.
* Client should be able to choose (at application start) which protocol should be used for interactions with server. Client should also be able to send multiple messages without a need to restart the application or to reconnect to server.

**Pay due attention to:**

* The ease of building process; the quality of applications' UX; their ability to durable and stable operations; their ability to exit correctly.
* Network interaction specifics are that many things can go wrong: errors, incorrect data, unexpected behavior of the remote side etc. Applications should be ready for that, so pay attention to the differences between TCP and UDP.
* Try to make the implementation as much robust and efficient as possible e.g., try to avoid using threads etc.

## Building and running the code

### System requirements

* Linux OS
* CMake 3.8+
* C++ compiler supporting C++11
 
The implementation was tested in the following environment: Ubuntu 20.04, GCC 9.3, CMake 3.13.
 
### Building
 
To build the project use the following snippets:
```bash
mkdir _build
cd _build
cmake ..
cmake --build . --target install
``` 

After building, there will be the `_stage/` folder containing three executables:
* `server` - the server itself; protocol, port and other parameters are defined with command line arguments
* `client` - the client; parameters are also defined with command line arguments
* `smoke_test` - simple test checking operability of both client and server (including concurrent connections)

Each executable provides basic docstring describing its parameters.

### Usage example

#### TCP (outputs for UDP are similar)

Terminal #1:
```bash
george@george:~/socket_demo/_stage$ ./server 8888 TCP 
```

Terminal #2:
```bash
george@george:~/socket_demo/_stage$ ./client 127.0.0.1 8888 TCP
Enter your message: hello
hello
Enter your message: 1 15 60
1 15 60
76
```

#### Smoke-testing

Terminal #1:
```bash
george@george:~/socket_demo/_stage$ ./server 8888 TCP 
```

Terminal #2:
```bash
george@george:~/socket_demo/_stage$ ./smoke_test 127.0.0.1 8888 TCP 5 1024
OK!
```

## Comments on the implementation

* The implementation of server is single-threaded, and the smoke-test checks server's ability to serve multiple clients (response correctness and its receival guarantee).
* The implementation limits the maximum message length to 65507 bytes (maximum data length in a single UDP datagram). This limitation is introduced to avoid ARQ protocol
implmentation on top of the UDP. This limitation is entirely artificial for TCP implementation, and introduced for the sole purpose of interface consistency.
* Server cannot operate using both TCP and UDP simultaneously - this can be only achieved with multiple `server` instances. There were no obstacles of implementing
such a feature, but I didn't interpret the problem statement this way.
* Smoke-test is recommended to be launched with small `operations_timeout_s` (1) and large `operations_timeout_s`
(w.r.t. `operations_timeout_s`). Otherwise the test fails for obvious reasons.
