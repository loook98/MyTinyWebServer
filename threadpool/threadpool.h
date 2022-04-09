#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool
{
private:
    int m_threadNumber; //线程池中线程数量
    int m_maxRequest;   //请求队列中允许的最大请求数

    pthread_t *m_threads;  //表示线程池的数组，大小为m_threadNumber
    list<T *> m_workQueue; //请求队列

    locker m_queueLocker; //保护请求队列的互斥量
    sem m_queueStat;      //表示队列中是否有作业

    connection_pool m_connPool; //数据库连接池

    int m_actorModel; //反应堆采用的模式
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

template <typename T>
threadpool<T>::threadpool(int model, connection_pool *connPool, int threadNumber = 8, int maxRequest = 10000)
{
    if (threadNumber <= 0 || maxRequest <= 0)
    {
        throw exception();
    }

    // m_actorModel = model;
    // this->m_connPool = connPool; //TODO 这里为什么不初始化这两个

    m_threads = new pthread_t[threadNumber];
    if (m_threads == nullptr)
    {
        throw exception();
    }

    for (int i = 0; i < threadNumber; i++)
    {
        if (pthread_create(&m_threads[i], nullptr, worker, this) != 0)
        {
            delete[] m_threads;
            throw exception();
        }

        if (pthread_detach(m_threads[i]) != 0)
        {
            delete[] m_threads;
            throw exception("pthread_detch error!\n");
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    // m_stop = true;
}

template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queueLocker.lock();
    if (m_workQueue.size() >= m_maxRequest)
    {
        m_queueLocker.unlock();
        return false;
    }

    request->m_state = state;
    m_workQueue.push_back(request);
    m_queueLocker.unlock();
    m_queueStat.post();
    
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queueLocker.lock();
    if (m_workQueue.size() >= m_maxRequest)
    {
        m_queueLocker.unlock();
        return false;
    }

    m_workQueue.push_back(request);
    m_queueLocker.unlock();
    m_queueStat.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg) //TODO 不太懂
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run()
{
    while (true)
    {
        m_queueStat.wait();
        m_queueLocker.lock();

        if (m_workQueue.empty())
        {
            m_queueLocker.unlock();
            continue;
        }

        T *request = m_workQueue.front();
        m_workQueue.pop_front();
        m_queueLocker.unlock();

        if (!request)
            continue;

        if (1 == m_actorModel)  //TODO 往后这些判断太懂
        {
            if (0 == request->m_state)
            {
                if (request->read_once())
                {
                    request->improv = 1;
                    connectionRAII mysqlconn(&request->msql, m_connPool);
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->time_flag = 1;
                }
            }
            else
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}

#endif