#include "sql_connection_pool.h"

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* connection_pool::getConnection() {
    MYSQL *conn = nullptr;
    if (0 == connList.size())
    {
        return nullptr;
    }
    
    reserve.wait();

    lock.lock();

    conn = connList.back();
    connList.pop_back();

    --m_freeConn;
    ++m_curConn;

    lock.unlock();
    return conn;
}

//释放一个连接，然后添加到池中
bool connection_pool::releaseConnection(MYSQL* conn) {
    if (nullptr == conn)
    {
        return false;
    }

    lock.lock();

    connList.push_back(conn);
    ++m_freeConn;
    --m_curConn;

    lock.unlock();
    reserve.post();

    return true;
}

//销毁连接池
void connection_pool::destroyPool() {
    lock.lock();

    if(connList.size() > 0){
        for(auto conn : connList){
            mysql_close(conn);
        }
        m_curConn = 0;
        m_freeConn = 0;
        connList.clear();
    }

    lock.unlock();
}

//当前的空闲连接数
int connection_pool::getFreeConn() {
    return this->m_freeConn;
}

//获得一个连接池的单例实例
connection_pool* connection_pool::getInstance() {
    static connection_pool conn_pool;
    return &conn_pool;
}

//构造初始化
void connection_pool::init(string url, string user, string password, string databaseName, int port, int maxConn, int close_log) {
    m_url = url;
    m_port = port; //TODO int赋给string？ 并且后边好像也没用到m_port
    m_user = user;
    m_password = password;
    m_databaseName = databaseName;
    m_close_log = close_log;

    for (int i = 0; i < maxConn; i++)
    {
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);
        if (nullptr == conn)
        {
            LOG_ERROR("MySQL error!");
            exit(1);
        }

        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), databaseName.c_str(), port, NULL, 0);
        if (nullptr == conn)
        {
            LOG_ERROR("MySQL error!");
            exit(1);
        }

        connList.push_back(conn);
        ++m_freeConn;
    }
    
    reserve = sem(m_freeConn);
    m_maxConn = maxConn;
}

connection_pool::connection_pool() {
    m_curConn = 0;
    m_freeConn = 0;
}

connection_pool::~connection_pool() {
    destroyPool();
}

connectionRAII::connectionRAII(MYSQL** conn, connection_pool *connPool) {
    *conn =  connPool->getConnection();

    connRAII = *conn;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
    poolRAII->releaseConnection(connRAII);
}
