#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//封装信号量的类
class sem
{
private:
    sem_t m_sem;

public:
    // 创建并初始化信号量
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
            throw std::exception();
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    // 销毁信号量
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    // 等待信号量
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    // 增加信号量
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
};

// 封装互斥锁的类
class locker
{
private:
    pthread_mutex_t m_mutex;

public:
    //创建并初始化互斥锁
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
            throw std::exception();
    }
    // 销毁互斥锁
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    // 获取互斥锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    // 释放互斥锁
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    //得到只想locker内部的pthread互斥量类型的指针
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
};

// 封装条件变量的类
class cond
{
private:
    //pthread_mutex_t m_mutex; //使用传进来的互斥锁
    pthread_cond_t m_cond;

public:
    // 创建并初始化条件变量
    cond()
    {
        // if (pthread_mutex_init(&m_mutex, NULL) != 0)
        //     throw std::exception();

        if (pthread_cond_init(&m_cond, NULL) != 0){
            //构造函数中一旦出错，就必须立即释放已分配了的资源（***）
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    // 销毁条件变量
    ~cond(){
        //pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    // 等待条件变量
    bool wait(pthread_mutex_t *m_mutex){
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    bool timedwait(pthread_mutex_t *m_mutex, struct timespec t){
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }

    // 唤醒等待的条件变量
    bool signal(){ 
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }
};

#endif