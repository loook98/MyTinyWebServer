#ifndef HTTP_CONN_H
#define HTTP_CONN_H

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
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn {
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    //报文的请求方法
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //主状态机的状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    //从状态机的状态
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    //报文解析的结果
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

public:
    http_conn() {}

    ~http_conn() {}

public:
    //初始化套接字地址。内部会调用私有的init方法
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);

    //关闭http连接
    void close_conn(bool real_close = true);

    void process();

    //读取浏览器端发来的全部数据
    bool read_once();

    //响应报文写入socket中发出去
    bool write();

    sockaddr_in *get_address() {
        return &m_address;
    }

    //同步线程初始化数据库读取表
    void initmysql_result(connection_pool *connPool);

//    //CGI使用线程池初始化数据库表
//    void initresultFile(connection_pool *connPool);

    int timer_flag;
    int improv;

private:
    void init();

    //从m_read_buf读取，并处理请求报文
    HTTP_CODE process_read();

    //向m_write_buf中写入响应报文数据
    bool process_write(HTTP_CODE ret);

    //主状态机中用于解析报文中请求行数据的函数
    HTTP_CODE parse_request_line(char *text);

    //主状态机中用于解析报文中请求头数据的函数
    HTTP_CODE parse_headers(char *text);

    //主状态机中用于解析报文中请求内容的函数
    HTTP_CODE parse_content(char *text);

    //生成响应报文
    HTTP_CODE do_request();

    //m_start_line是已解析的字符数
    //get_line用于将指针向后偏移，指向未处理的字符
    char *get_line() { return m_read_buf + m_start_line; }; //TODO 这里还要‘;’吗

    //从状态机。读取一行，分析是请求报文的哪一部分
    LINE_STATUS parse_line();

    void unmap();

    //根据响应报文格式，生成对应8个部分（放入缓冲区中），以下函数均由do_request函数调用
    bool add_response(const char *format, ...);

    bool add_content(const char *content);

    bool add_status_line(int status, const char *title);

    bool add_headers(int content_length);

    bool add_content_type();

    bool add_content_length(int content_length);

    bool add_linger();

    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state; //读为0， 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;

    //存储读取的请求报文数据
    char m_read_buf[READ_BUFFER_SIZE];
    //缓冲区m_read_buf中数据的最后一个字节的下一个位置
    int m_read_idx;
    //m_read_buf中正在读取的位置
    int m_checked_idx;
    //m_read_buf中已解析的字符个数
    int m_start_line;

    //存储发出的响应报文数据
    char m_write_buf[WRITE_BUFFER_SIZE];
    //指示buffer中的长度
    int m_write_idx;

    //主状态机当前状态
    CHECK_STATE m_check_state;
    //请求方法
    METHOD m_method;

    //以下为解析请求报文中对应的6个变量
    //存储读取文件的名称
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;

    char *m_file_address; //请求的文件用mmap映射到内存中的地址
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;  //是否其用的POST  TODO 这个变量干啥的
    char *m_string;   //存储请求头数据
    int bytes_to_send; //剩余发送字节数
    int bytes_have_send;   //已发送字节数
    char *doc_root;     //服务器上存放文件的根目录

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif //HTTP_CONN_H
