#include "lst_timer.h"
#include "../http/http_conn.h"

//sort_timer_lst类

sort_timer_lst::sort_timer_lst() {
    head = nullptr;
    tail = nullptr;
}

sort_timer_lst::~sort_timer_lst() {
    util_timer *tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer) {
    if (!timer)
        return;

    if (!head){
        head = tail = timer;
        return;
    }

    if (timer->expire < head->expire){
        head->prev = timer;
        timer->next = head;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

void sort_timer_lst::adjust_timer(util_timer *timer) {
    if (!timer){
        return;
    }

    //如果timer是尾部节点或者timer的过期时间小于他后边的定时器，不做调整
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire)){
        return;
    }

    //如果timer是头结点，则取出，重新插入
    if (timer == head){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    } else{ //timer在节点内部，则取出，重新插入
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer) {
    if (!timer)
        return;

    //链表中只有一个节点
    if ((timer == head) && (timer == tail)){
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }

    //被删除的定时器是头节点
    if (timer == head){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }

    //被删除的定时器是尾节点
    if (timer == tail){
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }

    //被删除的定时器在链表内部，常规链表节点的删除
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_timer_lst::tick() {
    if (!head){
        return;
    }

    //获取当前时间
    time_t cur = time(nullptr);
    util_timer *tmp = head;

    //遍历定时器链表
    while (tmp){
        //遇到没有过期的定时器则停止
        if (cur < tmp->expire)
            break;

        //若当前的定时器过期，则调用回调函数，并删除该定时器
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head){
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head) {
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;

    //遍历lst_head之后的节点，按照超时时间找到目标定时器对应的位置，然后插入
    while (tmp){
        if (timer->expire < tmp->expire){
            prev->next = timer;
            timer->prev = prev;
            tmp->prev = timer;
            timer->next = tmp;
            break;
        }
        prev = tmp;
        tmp = tmp->next;

    }

    //遍历完还未找到过期时间大于timer的定时器，则将目标定时器放荡链表尾部
    if (!tmp){
        prev->next= timer;
        timer->prev = prev;
        timer->next = nullptr;
        tail = timer;
    }
}

//Utils类

void Utils::init(int timeslot) {
    m_TIMESLOT = timeslot;
}

int Utils::setNonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
}

void Utils::sig_handler(int sig) {
    //为保证函数的可重入性。备份原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号对应的处理函数
void Utils::addsig(int sig, void (*handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;

    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}


void Utils::timer_handler() {
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info) {
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data){
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
