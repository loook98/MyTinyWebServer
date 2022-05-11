#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template<typename T>
class threadpool {
private:
    int m_threadNumber; //线程池中线程数量
    int m_maxRequest;   //请求队列中允许的最大请求数

    pthread_t *m_threads;  //表示线程池的数组，大小为m_threadNumber
    list<T *> m_workQueue; //请求队列

    locker m_queueLocker; //保护请求队列的互斥量
    sem m_queueStat;      //表示队列中是否有作业

    connection_pool *m_connPool; //数据库连接池

    int m_actorModel; //事件处理模式切换（Reactor或Proactor）
    // bool m_stop; //是否结束线程
private:
    static void *worker(void *arg);

    void run();

public:
    threadpool(int model, connection_pool *connPool, int threadNumber = 8, int maxRequest = 10000);

    ~threadpool();

    bool append(T *request, int state);

    bool append_p(T *request);
};

template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int threadNumber, int maxRequest)
        :m_actorModel(actor_model), m_threadNumber(threadNumber), m_maxRequest(maxRequest),
         m_threads(NULL), m_connPool(connPool) {
    if (threadNumber <= 0 || maxRequest <= 0) {
        throw exception();
    }

    m_threads = new pthread_t[m_threadNumber];
    if (m_threads == nullptr) {
        throw exception();
    }

    for (int i = 0; i < threadNumber; i++) {
        if (pthread_create(&m_threads[i], nullptr, worker, this) != 0) {
            delete[] m_threads;
            throw exception();
        }

        if (pthread_detach(m_threads[i]) != 0) {
            delete[] m_threads;
            throw exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    // m_stop = true;
}

//Reactor模式用的插入工作的函数，state 0表示插入的任务为读，1表示插入的任务为写
template<typename T>
bool threadpool<T>::append(T *request, int state) {
    m_queueLocker.lock();
    if (m_workQueue.size() >= m_maxRequest) {
        m_queueLocker.unlock();
        return false;
    }

    request->m_state = state;
    m_workQueue.push_back(request);
    m_queueLocker.unlock();
    m_queueStat.post();

    return true;
}

//Proactor模式使用的append
template<typename T>
bool threadpool<T>::append_p(T *request) {
    m_queueLocker.lock();
    if (m_workQueue.size() >= m_maxRequest) {
        m_queueLocker.unlock();
        return false;
    }

    m_workQueue.push_back(request);
    m_queueLocker.unlock();
    m_queueStat.post();
    return true;
}

template<typename T>
void *threadpool<T>::worker(void *arg) //传给pthread_create的静态方法。内部调用run方法。
{
    //调用pthread_create时给worker函数传入的参数是this，所以该操作其实是获取线程池对象的地址。
    threadpool *pool = (threadpool *) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    while (true) {
        m_queueStat.wait();
        m_queueLocker.lock();

        if (m_workQueue.empty()) {
            m_queueLocker.unlock();
            continue;
        }

        T *request = m_workQueue.front();
        m_workQueue.pop_front();
        m_queueLocker.unlock();

        if (!request)
            continue;

        //1表示Reactor模式
        if (1 == m_actorModel)
        {
            if (0 == request->m_state) {  //0表示从请求队列中取出的是需要读的事件
                if (request->read_once()) {
                    request->improv = 1;
                    connectionRAII mysqlconn(&request->msql, m_connPool); //从数据库连接池中取一个连接，并且在mysqlconn这个变量生命周期结束时会自动释放取出的连接。
                    request->process();
                } else {
                    request->improv = 1;
                    request->time_flag = 1;
                }
            } else { //否则是1，表示从请求队列中取出的是需要写的事件
                if (request->write()) {
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else { //Proactor模式。线程池不需要进行数据读取，而是直接开始业务处理。之前的操作已经将数据读取到http的read和write的buffer中了。
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}

#endif