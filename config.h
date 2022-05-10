#ifndef CONFIG_H
#define CONFIG_H

#include "webserver.h"

using namespace std;

class Config {
public:
    Config();

    ~Config();

    void parse_arg(int argc, char *argv[]);

    int PORT;       //端口号
    int LOGWrite;   //日志写入方式
    int TRIGMode;   //对listenfd和connfd采用的触发方式的组合
    int LISTENTrigmode;  //listenfd的触发模式
    int CONNTrigmode;    //connfd的触发模式
    int OPT_LINGER;      //优雅关闭连接
    int sql_num;         //数据库连接池数量
    int thread_num;      //线程池内的线程数量
    int close_log;       //是否关闭日志
    int actor_model;     //并发模式的选择
};

#endif //CONFIG_H
