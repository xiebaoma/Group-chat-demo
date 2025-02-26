#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <vector>

#define MAX_EVENTS 10
#define PORT 8080

// 设置文件描述符为非阻塞
int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        perror("fcntl");
        return -1;
    }
    return 0;
}

// 处理客户端消息
void handleClientMessage(int clientFd)
{
    char buffer[1024];
    ssize_t bytesRead;

    while (true)
    {
        // 从客户端读取数据，尽可能多地读取数据
        bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
        if (bytesRead < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("read");
                close(clientFd);
            }
            break; // 如果没有更多数据可读，则退出
        }
        else if (bytesRead == 0)
        {
            // 客户端关闭连接
            std::cout << "Client disconnected" << std::endl;
            close(clientFd);
            break;
        }
        else
        {
            // 输出客户端消息并回送
            buffer[bytesRead] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            send(clientFd, buffer, bytesRead, 0);
        }
    }
}

int main()
{
    int serverFd, clientFd, epollFd;
    struct sockaddr_in serverAddr;
    struct epoll_event event, events[MAX_EVENTS];

    // 创建服务器套接字
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1)
    {
        perror("socket");
        return -1;
    }

    // 设置套接字地址结构
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("bind");
        close(serverFd);
        return -1;
    }

    // 监听连接
    if (listen(serverFd, 5) == -1)
    {
        perror("listen");
        close(serverFd);
        return -1;
    }

    // 设置服务器套接字为非阻塞
    if (setNonBlocking(serverFd) == -1)
    {
        close(serverFd);
        return -1;
    }

    // 创建 epoll 实例
    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        perror("epoll_create1");
        close(serverFd);
        return -1;
    }

    // 注册监听事件到 epoll，使用边缘触发
    event.events = EPOLLIN | EPOLLET; // 可读事件，边缘触发
    event.data.fd = serverFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1)
    {
        perror("epoll_ctl");
        close(serverFd);
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // 主循环，等待并处理事件
    while (true)
    {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < numEvents; i++)
        {
            if (events[i].data.fd == serverFd)
            {
                // 新客户端连接
                clientFd = accept(serverFd, nullptr, nullptr);
                if (clientFd == -1)
                {
                    perror("accept");
                    continue;
                }

                std::cout << "New client connected" << std::endl;

                // 设置客户端为非阻塞
                if (setNonBlocking(clientFd) == -1)
                {
                    close(clientFd);
                    continue;
                }

                // 注册客户端套接字到 epoll，使用边缘触发
                event.events = EPOLLIN | EPOLLET; // 可读事件，边缘触发
                event.data.fd = clientFd;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
                {
                    perror("epoll_ctl");
                    close(clientFd);
                    continue;
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                // 客户端发送数据
                handleClientMessage(events[i].data.fd);
            }
        }
    }

    // 清理
    close(serverFd);
    close(epollFd);
    return 0;
}
