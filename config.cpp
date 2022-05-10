#include "config.h"

Config::Config() {
    //默认端口9006
    PORT = 9006;

    //日志写入方式，默认同步
    LOGWrite = 0;

    //触发模式组合，默认listenfd LT + connfd LT
    TRIGMode = 0;

    //listenfd触发模式，默认LT
    LISTENTrigmode = 0;

    //connfd触发默认，默认LT
    CONNTrigmode = 0;

    //优雅关闭连接，默认不使用
    OPT_LINGER = 0;

    //数据库连接池数量,默认8
    sql_num = 8;

    //线程池内的线程数量,默认8
    thread_num = 8;

    //关闭日志,默认不关闭
    close_log = 0;

    //并发模型,默认是proactor
    actor_model = 0;
}

//在main函数中调用，负责解析命令行参数
void Config::parse_arg(int argc, char **argv) {
    int opt;
    //-p:自定义端口号; -l:日志写入方式； -m:触发模式的组合；
    //-o:优雅关闭连接; -s:数据库连接数量； -t:线程数量
    //-c：关闭日志；  -a：选择反应堆模型
    const char *str = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, str)) != -1){
        switch (opt) {
            case 'p':{
                PORT = atoi(optarg);
                break;
            }
            case 'l':{
                LOGWrite = atoi(optarg);
                break;
            }
            case 'm':
            {
                TRIGMode = atoi(optarg);
                break;
            }
            case 'o':
            {
                OPT_LINGER = atoi(optarg);
                break;
            }
            case 's':
            {
                sql_num = atoi(optarg);
                break;
            }
            case 't':
            {
                thread_num = atoi(optarg);
                break;
            }
            case 'c':
            {
                close_log = atoi(optarg);
                break;
            }
            case 'a':
            {
                actor_model = atoi(optarg);
                break;
            }
            default:
                break;
        }
    }
}