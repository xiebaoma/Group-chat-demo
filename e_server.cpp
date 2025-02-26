#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <vector>
#include <algorithm>

#define MAX_EVENTS 10
#define PORT 12345
#define BUFFER_SIZE 1024

// 设置非阻塞模式
int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

int main()
{
    int server_fd, epoll_fd;
    struct sockaddr_in server_addr;
    struct epoll_event event, events[MAX_EVENTS];

    // 创建服务器端 socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed");
        return -1;
    }

    // 设置服务器端地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定地址
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(server_fd);
        return -1;
    }

    // 监听连接请求
    if (listen(server_fd, 10) == -1)
    {
        perror("listen failed");
        close(server_fd);
        return -1;
    }

    // 创建 epoll 实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1 failed");
        close(server_fd);
        return -1;
    }

    // 将服务器套接字加入 epoll 实例，监听可读事件
    setNonBlocking(server_fd);
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET; // 边缘触发
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        perror("epoll_ctl failed");
        close(server_fd);
        return -1;
    }

    std::vector<int> client_fds;

    // 进入事件循环
    while (true)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == server_fd)
            {
                // 新客户端连接请求
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1)
                {
                    perror("accept failed");
                    continue;
                }

                // 设置非阻塞模式
                setNonBlocking(client_fd);

                // 将新客户端加入 epoll 监听
                event.data.fd = client_fd;
                event.events = EPOLLIN | EPOLLET; // 边缘触发
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
                {
                    perror("epoll_ctl failed");
                    close(client_fd);
                    continue;
                }

                // 保存客户端文件描述符
                client_fds.push_back(client_fd);
                std::cout << "New client connected: " << client_fd << std::endl;
            }
            else if (events[i].events & EPOLLIN)
            {
                // 读取客户端数据
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                // ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

                // 边缘触发模式下，必须尽量读取所有数据
                while (true)
                {
                    int bytes_read = read(client_fd, buffer, sizeof(buffer));
                    if (bytes_read == 0)
                    {
                        // 客户端断开连接
                        close(client_fd);
                        break;
                    }
                    else if (bytes_read < 0)
                    {
                        // 错误发生或没有数据
                        if (errno == EAGAIN)
                        {
                            break; // 数据读取完毕
                        }
                        else
                        {
                            perror("read");
                            close(client_fd);
                            break;
                        }
                    }
                    else
                    {
                        // 处理读取的数据
                    }
                }

                if (bytes_read == -1)
                {
                    perror("read failed");
                    continue;
                }
                else if (bytes_read == 0)
                {
                    // 客户端断开连接
                    std::cout << "Client disconnected: " << client_fd << std::endl;
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
                    continue;
                }

                // 将接收到的数据转发给所有客户端
                for (int fd : client_fds)
                {
                    if (fd != client_fd)
                    { // 不发送给自己
                        if (send(fd, buffer, bytes_read, 0) == -1)
                        {
                            perror("send failed");
                        }
                    }
                }
            }
        }
    }

    // 清理
    close(server_fd);
    close(epoll_fd);
    return 0;
}
