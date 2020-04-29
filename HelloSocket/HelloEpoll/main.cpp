#include<unistd.h> //uni std
#include<arpa/inet.h>
#include<string.h>
#include "CELLEpoll.hpp"

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)


#include<stdio.h>
#include<vector>
#include<map>
#include<thread>
#include<mutex>
#include<atomic>
#include<thread>
#include<algorithm>

std::vector<SOCKET> g_clients;
bool g_bRun = true;
int msgCount = 0;

void cmdThread(){
    while(1){
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if (0 == strcmp(cmdBuf, "exit")){
            g_bRun = false;
            printf("退出cmdThread\n");
            break;
        }else{
            printf("不支持命令\n");
        }
    }
}

// 缓冲区
char g_szBuff[4096] = {};
int g_nLen = 0;
int recvData(SOCKET cSock){
    // 接收数据
    g_nLen = (int)recv(cSock, g_szBuff, sizeof(g_szBuff)/sizeof(char), 0);
    return g_nLen;
}

int WriteData(SOCKET cSock){
    if (g_nLen > 0){
        int nLen = (int)send(cSock, g_szBuff, g_nLen, 0);
        return nLen;
    }
    return -1;
}

int clientLeave(SOCKET cSock){
    close(cSock);
    printf("client sock = <%d> closed\n", cSock);
    auto iter = std::find(g_clients.begin(), g_clients.end(), cSock);
    if (g_clients.end() != iter){
        g_clients.erase(iter);
    }
}

int main(){
    // 线程
    std::thread t1(cmdThread);
    t1.detach();
    // 建立套接字
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 绑定端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
    _sin.sin_addr.s_addr = INADDR_ANY;
    if (SOCKET_ERROR == bind(_sock, (sockaddr *)&_sin, sizeof(_sin))){
        printf("绑定端口失败\n");
    }else{
        printf("绑定端口成功\n");
    }
    // 监听套接字
    if (SOCKET_ERROR == listen(_sock, 5)){
        printf("监听端口失败\n");
    }else{
        printf("监听端口成功\n");
    }
    
    const int maxClient = 60000;
    CELLEpoll ep;
    ep.create(maxClient);
    // 本服务器监听的断开添加到事件管理器
    ep.ctl(EPOLL_CTL_ADD, _sock, EPOLLIN);

    int msgCount = 0;
    int cCount = 0;

    while(g_bRun){
        int n = ep.wait(1);
        // epoll 会返回所有触发事件的文件描述符
        auto events = ep.events();
        // 但是事件集合中有些是读事件，有些是写事件，依然需要做区分。
        for (int i = 0; i < n; ++i){
            // 本服务器监听的端口有可读数据，表示有新客户端接入
            if (events[i].data.fd == _sock){
                // 再次判断一下事件类型，确定是新客户端加入
                if (events[i].events & EPOLLIN){
                    // 准备建立与新客户端的连接
                    sockaddr_in clientAddr = {};
                    int nAddrLen = sizeof(sockaddr_in);
                    SOCKET _cSock = INVALID_SOCKET;
                    _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
                    ++cCount;  // 客户端计数
                    if (_cSock == INVALID_SOCKET){
                        perror("accept INVALID client socket");
                    }else{
                        g_clients.push_back(_cSock);
                        ep.ctl(EPOLL_CTL_ADD, _cSock, EPOLLIN);
                        printf("new client joined:sock = %d, IP = %s\n",
                        (int)_cSock, inet_ntoa(clientAddr.sin_addr));
                    }
                    continue;
                    // 读事件触发的情况：有新连接，有数据传入，有客户端断开
                }
            }
            if (events[i].events & EPOLLIN){
                int cSockfd = events[i].data.fd;
                int ret = recvData(cSockfd);
                if (ret <= 0){
                    clientLeave(cSockfd);
                    printf("client<sock = %d> quitted\n", cSockfd);            
                }else{
                printf("recv client num%d msg:sock = %d, len = %d\n",
                    ++msgCount, events[i].data.fd, ret);
                }
                // 接收到数据后，将对应的客户端描述符改为读（这样子操作只是为了测试epoll的读写事件而言）
                ep.ctl(EPOLL_CTL_MOD, cSockfd, EPOLLOUT);
            }// 可写事件触发
            else if (events[i].events & EPOLLOUT){
                printf("EPOLLOUT %d\n", msgCount);
                auto cSock = events[i].data.fd;
                int ret = WriteData(cSock);
                if (0 >= ret){
                    clientLeave(cSock);
                }
                if (5 > msgCount){
                    ep.ctl(EPOLL_CTL_MOD, cSock, EPOLLIN);
                }else{
                    ep.ctl(EPOLL_CTL_DEL, cSock, 0);
                }
            }
            // if (events[i].events & EPOLLERR){ // 这个是错误事件会和可读事件重复触发触发
            //     int cSockfd = events[i].data.fd;
            //     printf("EPOLLERR : id = %d, socket = %d\n", msgCount, cSock);
            // }
            // if (events[i].events & EPOLLHUP){  // 这个是断开事件会和可读事件重复触发触发
            //     int cSockfd = events[i].data.fd;
            //     printf("EPOLLHUP : id = %d, socket = %d\n", msgCount, cSock);
            // }
            // }else   
        }
    }
    for (auto client : g_clients){
        close(client);
    }
    close(_sock);
    printf("quit \n");
    return 0;
}


