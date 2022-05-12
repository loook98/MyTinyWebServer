#ifndef LST_TIMER_H
#define LST_TIMER_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"


class util_timer;

//连接资源
struct client_data {
    //客户端socket地址
    sockaddr_in address;

    //socket文件描述符
    int sockfd;

    //定时器
    util_timer *timer;
};

//定时器
class util_timer {
public:
    util_timer() : prev(nullptr), next(nullptr) {}

public:
    time_t expire;
    //指向定时器回调函数的函数指针
    void (*cb_func)(client_data *);

    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

//定时容器
class sort_timer_lst {
public:
    sort_timer_lst();
    //销毁链表
    ~sort_timer_lst();

    //添加定时器,内部调用私有的add_timer
    void add_timer(util_timer *timer);
    //调整定时器timer在链表中的位置
    void adjust_timer(util_timer *timer);
    //删除定时器
    void del_timer(util_timer *timer);
    //定时器容器的定时事件处理函数。找到容器（链表）中所有过期的定时器，调用其cb_func回调函数。
    void tick();

private:
    //私有成员，被公有成员add_timer和adjust_time调用
    //主要用于调整链表内部结点.将目标定时器timer添加到lst_head之后的部分链表中.
    void add_timer(util_timer *timer, util_timer *lst_head);

private:
    util_timer *head;
    util_timer *tail;
};

class Utils{
public:
    Utils(){}
    ~Utils(){}

    void init(int timeslot);
    //对文件描述符设置非阻塞
    int setNonblocking(int fd);

    //向内核事件表注册时间，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理函数，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

//定时器回调函数
void cb_func(client_data *user_data);

#endif //LST_TIMER_H
