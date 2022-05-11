#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class log{
public:
    static log *get_instance(){
        static log instance;
        return &instance;
    }

    static void *flush_log_thread(void *arg){
        log::get_instance()->async_write_log(); //TODO 这里不用返回吗
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    //将输出内容按照标准格式整理
    void write_log(int level, const char *format, ...);

    //强制刷新缓冲区
    void flush(void );

private:
    log();
    virtual ~log();

    void *async_write_log(){ //TODO 返回值为void* ? 这里也没return啊
        string single_log;
        //从阻塞队列中取出一个日志string，写入日志文件
        while (m_log_queue->pop(single_log)){
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128]; //路径名
    char log_name[128]; //文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count;  //记录日志行数
    int m_today;        //因为按天分类，记录当前时间是哪一天
    FILE *m_fp;         //打开log的文件指针
    char *m_buf;
    block_queue<string>  *m_log_queue;   //阻塞队列
    bool m_is_async;                     //异步还是同步写入
    locker m_mutex;                      //互斥量
    int m_close_log;                    //关闭日志系统
};

//这四个宏定义在其他文件中使用，主要用于不同类型的日志输出
#define LOG_DEBUG(format, ...) if(m_close_log == 0) { log::get_instance()->write_log(0, format, ##__VA_ARGS__); \
                                                      log::get_instance()->flush(); }
#define LOG_INFO(format, ...) if(m_close_log == 0) { log::get_instance()->write_log(1, format, ##__VA_ARGS__); \
                                                     log::get_instance()->flush(); }
#define LOG_WARN(format, ...) if(m_close_log == 0) { log::get_instance()->write_log(2, format, ##__VA_ARGS__); \
                                                     log::get_instance()->flush(); }
#define LOG_ERROR(format, ...) if(m_close_log == 0) { log::get_instance()->write_log(3, format, ##__VA_ARGS__); \
                                                      log::get_instance()->flush(); }

#endif //LOG_H
