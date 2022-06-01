//循环数组实现的阻塞队列
//线程安全，每个操作前都要先都要先加互斥锁，操作完后，再解锁.
//生产者-消费者模型

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <ostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"

using namespace std;

template<typename T>
class block_queue {
public:
    block_queue(int maxSize = 1000) {
        if (maxSize <= 0)
            exit(-1);

        m_maxSize = maxSize;
        m_array = new T[m_maxSize];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    ~block_queue() {
        m_mutex.lock();

        if (m_array != nullptr)
            delete[] m_array;

        m_mutex.unlock();
    }

    void clear() {
        m_mutex.lock();

        m_size = 0;
        m_front = -1;
        m_back = -1;

        m_mutex.unlock();
    }

    //判断队列是否满
    bool full() {
        m_mutex.lock();
        if (m_maxSize < m_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    //判断队列是否为为空
    bool empty() {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    //通过参数value返回队首元素
    bool front(T &value) {
        m_mutex.lock();
        if (0 == m_size) {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    //通过参数value返回队尾元素
    bool back(T &value) {
        m_mutex.lock();
        if (0 == m_size) {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    //返回队列大小
    int size() {
        int ret = 0;
        m_mutex.lock();
        ret = m_size;
        m_mutex.unlock();
        return ret;
    }

    //返回队列最大容量
    int maxSize() {
        int ret = 0;
        m_mutex.lock();
        ret = m_maxSize;
        m_mutex.unlock();
        return ret;
    }

    //往队列中添加元素
    bool push(const T &value) {
        m_mutex.lock();
        if (m_size >= m_maxSize) {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_maxSize;
        m_array[m_back] = value;

        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    //从队首取元素,得到的元素放入value中。如果队列为空需一直等待条件变量
    bool pop(T &item) {
        m_mutex.lock();
        while (m_size <= 0) {
            if (!m_cond.wait(m_mutex.get())){
                m_mutex.unlock();
                return false;
            }
        }
        item = m_array[m_front]; //TODO 为啥原作者这里更新m_front后再给item赋值？难道(front,end]?
        m_front = (m_front + 1) % m_maxSize;
        m_size--;
        m_mutex.unlock();
        return true;
    }

    //pop增加了超时处理
    bool pop(T &item, int ms_timeout){
        timespec t = {0,0};
        timeval now = {0,0};

        gettimeofday(&now, nullptr);

        m_mutex.lock();
        while (m_size <= 0){
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!m_cond.timedwait(m_mutex.get(), t)){
                m_mutex.unlock();
                return false;
            }
        }

        if (m_size <= 0){
            m_mutex.unlock();
            return false;
        }

        item = m_array[m_front]; //TODO 为啥原作者这里更新m_front后再给item赋值？难道(front,end]?
        m_front = (m_front + 1) % m_maxSize;
        m_size--;
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;
    cond m_cond;

    T *m_array;    //实现队列的数组
    int m_maxSize; //队列的最大元素个数
    int m_size;    //队列当前元素个数
    int m_front;   //队列头部
    int m_back;    //队列尾部
};

#endif //BLOCK_QUEUE_H
