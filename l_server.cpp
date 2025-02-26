#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>

#define PORT 8080
#define MAX_EVENTS 10
#define BUF_SIZE 1024

// 设置非阻塞模式
int setNonBlocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl");
        return -1;
    }
    return 0;
}

int main()
{
    int server_fd, epoll_fd, nfds;
    struct sockaddr_in server_addr;
    struct epoll_event ev, events[MAX_EVENTS];
    std::vector<int> clients;

    // 创建服务器socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    // 设置非阻塞模式
    if (setNonBlocking(server_fd) == -1)
    {
        return -1;
    }

    // 初始化服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        return -1;
    }

    // 监听
    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        return -1;
    }

    // 创建epoll实例
    if ((epoll_fd = epoll_create1(0)) == -1)
    {
        perror("epoll_create1");
        return -1;
    }

    // 将服务器socket加入epoll监控
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    std::cout << "Server started. Waiting for clients to connect..." << std::endl;

    // 主循环
    while (true)
    {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            return -1;
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == server_fd)
            {
                // 处理新的连接
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_fd == -1)
                {
                    perror("accept");
                    continue;
                }

                // 设置新连接为非阻塞
                if (setNonBlocking(client_fd) == -1)
                {
                    close(client_fd);
                    continue;
                }

                // 将新客户端socket添加到epoll中
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }

                clients.push_back(client_fd);
                std::cout << "New client connected: " << client_fd << std::endl;
            }
            else
            {
                // 处理客户端发送的数据
                int client_fd = events[n].data.fd;
                char buffer[BUF_SIZE];
                int bytes_read = read(client_fd, buffer, BUF_SIZE);
                if (bytes_read == -1)
                {
                    perror("read");
                    continue;
                }
                else if (bytes_read == 0)
                {
                    // 客户端关闭连接
                    std::cout << "Client disconnected: " << client_fd << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                    close(client_fd);
                    clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
                }
                else
                {
                    // 广播消息给其他所有客户端
                    buffer[bytes_read] = '\0'; // 确保是一个有效的字符串
                    for (int fd : clients)
                    {
                        if (fd != client_fd)
                        {
                            if (write(fd, buffer, bytes_read) == -1)
                            {
                                perror("write");
                            }
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
