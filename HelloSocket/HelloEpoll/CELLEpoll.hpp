#ifndef _CELL_EPOLL_HPP_
#define _CELL_EPOLL_HPP_

#if __linux__

#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/epoll.h>
#include<cstdio>

#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define EPOLL_ERROR (-1)

class CELLEpoll{
/*
* 1. 创建事件管理器（红黑树）
* 2. 销毁事件管理器（红黑树）
* 3. 添加监听事件
* 4. 监听
*/
public:
    // 构造函数由默认构造函数提供
    // 析构
    ~CELLEpoll(){
        destory();
    }
    //  创建 epoll事件管理器 红黑树的根节点
    int create(int nMaxEvents){
        // 如果红黑树没有被销毁，先销毁
        if (_epfd > 0){
            destory();
        }
        _nMaxEvents = nMaxEvents;
        // linux 2.6.8 以后，最大能接收的客户端
        // 由内核自定义，输入值只作为参考
        _epfd = epoll_create(nMaxEvents);
        if (EPOLL_ERROR == _epfd){
            // linux 下的标准错误函数,自动回车
            perror("epoll_create");
        }
        // 向_epfd 注册一个监听事件
        // cell_epoll_ctl(_epfd, EPOLL_CTL_ADD, _sock, EPOLLIN);
        return _epfd;
    }

    void destory(){
        // 销毁 红黑树 和 事件集合
        if (_epfd > 0){
            close(_epfd);
            _epfd = -1;
        }
        if (_pEvents){
            delete[] _pEvents;
            _pEvents = nullptr;
        }
    }
    // 为 epoll事件管理器红黑树添加监听事件
    int ctl(int op, SOCKET sockfd, uint32_t events){
        epoll_event ev;
        ev.events = events;
        ev.data.fd = sockfd;

        int ret = epoll_ctl(_epfd, op, sockfd, &ev);
        if (EPOLL_ERROR == ret){
            perror("epoll_ctl error");
        }
        return ret;
    }

    int wait(int timeout){
        // _epfd : 事件管理器根节点文件描述符
        // events: 用于接收检测到的网络事件的数组
        // maxevents 接收数组的大小，能够接收的事件数量
        // timeout
        //      -1 阻塞
        //      0 非阻塞
        //      >0 等待设定的时间
        int ret = epoll_wait(_epfd, _pEvents, _nMaxEvents, 1);
        if (EPOLL_ERROR == ret){
            printf("_epfd = %d\n", _epfd);
            perror("error, epoll_wait");
        }
        return ret;
    }

    epoll_event* events(){
        return _pEvents;
    }
private:
    // c++11 允许类成员变量在声明时定义
    int _epfd = -1;
    // 存储触发事件的描述符集合
    epoll_event * _pEvents = nullptr;
    int _nMaxEvents = 256;
};


#endif // __linux__
#endif 
