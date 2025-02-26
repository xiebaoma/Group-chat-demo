# Group-chat-demo

This repository demonstrates a simple group chat application with both the client and server components, implemented in C++. The server utilizes `epoll` to handle multiple client connections with two different approaches: **level-triggered** and **edge-triggered** modes. The project is designed to run on a Linux system.

## Project Overview

The project includes:

- Server

  : A multi-client server that handles incoming connections from clients using 

  ```
  epoll
  ```

  . The server is implemented using two methods of I/O event notification:

  - **Level-triggered mode**: Where the server continuously checks for new data from clients as long as the data is available.
  - **Edge-triggered mode**: Where the server only reacts when a new event (e.g., new data or connection) is available, minimizing the need for repeated polling.

- **Client**: A simple client that connects to the server, sends and receives messages, and participates in the chat.

The server and client communication is based on basic socket programming principles, and the implementation demonstrates efficient, scalable handling of client connections on a Linux system.

## Features

- **Two I/O event handling modes**:
  - **Level-triggered mode**: Easy to implement, but may lead to redundant processing.
  - **Edge-triggered mode**: More efficient in handling events, but requires more careful management of socket state.
- **Group chat**: Clients can send and receive messages in real-time, simulating a simple group chat environment.

## Requirements

- Linux-based operating system (Ubuntu, CentOS, etc.)
- C++11 or higher
- `g++` or another compatible C++ compiler
- Basic knowledge of socket programming and `epoll`

## How to Build and Run

### Step 1: Clone the Repository

```bash
git clone https://github.com/yourusername/Group-chat-demo.git
cd Group-chat-demo
```

### Step 2: Build the Project

Compile the server and client programs using `g++`:

```bash
g++ -o l_server l_server.cpp -std=c++11
g++ -o e_server e_server.cpp -std=c++11
g++ -o client client.cpp -std=c++11
```

### Step 3: Run the Server

To start the server, run:

```bash
./l_server
./e_server
```

The server will begin listening for incoming client connections.

### Step 4: Run the Client

To start a client, run:

```bash
./client
```

You can run multiple instances of the client to simulate a group chat.

## Code Structure

- **l_server.cpp/e_server.cpp**: Implements the server-side logic, handling multiple clients using `epoll`.
- **client.cpp**: Implements the client-side logic for connecting to the server and sending/receiving messages.
- **README.md**: This file, which provides the instructions for using the project.

## Contributing

If you find bugs or want to improve the project, feel free to fork the repository and submit pull requests. Any contributions are welcome!

## License

This project is licensed under the MIT License - see the [LICENSE](https://chatgpt.com/c/LICENSE) file for details.
