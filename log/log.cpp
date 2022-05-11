#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>

using namespace std;

log::log() {
    m_count = 0;
    m_is_async = false;
}

log::~log() {
    if (m_fp)
        fclose(m_fp);
}

//init函数实现日志的创建、写入方式的判断
bool log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size) {
    //队列大小大于0，则为异步写入
    if (max_queue_size > 0) {
        m_is_async = true;

        //创建阻塞队列
        m_log_queue = new block_queue<string>(max_queue_size); //TODO 在哪里delete
        //创建异步写日志的线程
        pthread_t tid;
        pthread_create(&tid, nullptr, flush_log_thread, nullptr);
    }

    m_close_log = close_log;

    //输出内容的长度
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);

    //日志最大行数
    m_split_lines = split_lines;

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //p指向file_name中最后一个/的位置
    const char *p = strrchr(file_name, '/');
    char log_full_name[255] = {0};

    //生成日志名log_full_name
    //如输入的文件名没有/，则直接将时间和文件名作为日志名
    if (p == nullptr) {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                 file_name);
    } else { //有/的情况
        //将p向后移一位复制到log_name
        strcpy(log_name, p + 1);
        //赋值路径名到dir_name中. p - file_name + 1为 [file_name...p]之间长度。
        strncpy(dir_name, file_name, p - file_name + 1);

        snprintf(log_full_name, 255, "%s%d_%02d_%02d%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                 my_tm.tm_mday, file_name);
    }

    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a");
    if (m_fp == nullptr)
        return false;

    return true;
}

// 根据格式生成要写入的日志信息字符串。
// 然后将日志同步地写入文件，或者异步地写入阻塞队列的函数。
void log::write_log(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};

    //日志分级
    switch (level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[error]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }

    m_mutex.lock();
    //更新现有行数
    m_count++;
    //日志不是今天写入的或写入的日志行数是最大行的倍数
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};

        //格式化日志名中的时间部分
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon+1, my_tm.tm_mday);

        //如果不是今天的时间，则生成新日志，更新m_today和m_count
        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 256, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        } else { //写入的日志行数是最大行的倍数
            //超过了最大行数，在之前的日志名基础上加上后缀m_count/m_split_lines
            snprintf(new_log, 256, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list valst;
    //将传入format参数付给valst,以便格式化输出
    va_start(valst, format);

    string log_str;

    m_mutex.lock();
    //写入内容格式： 时间+内容
    //格式化时间，snprintf成功返回写字符的总数，其中不包括结尾的null字符
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    //格式化内容，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);

    m_buf[n + m] = '\n';
    m_buf[n+m+1] = '\0';

    log_str = m_buf;
    m_mutex.unlock();

    //若异步，则将日志信息加入阻塞队列；若同步，则加锁向文件写入
    if (m_is_async && !m_log_queue->full()){
        m_log_queue->push(log_str);
    } else{
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }
    va_end(valst);
}

void log::flush() {
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}
