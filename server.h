#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>

#define EV_MAX 128

using namespace std;

enum OPTYPE
{
    SYS_EXIT = 0,
    LOGIN,
    REGISTER,
    SHOW,
    YD,
    XSYD,
    QXYD
};

struct SerConf
{
    SerConf()
    {
        ips = "127.0.0.1";
        port = 6000;
        lismax = 128;
        taskmax = 128;
        threadnum = 4;
    }

    bool ReadConf(string filename); // 读取配置文件
    void PrintInfo();               // 打印读取的配置信息

    string ips;
    short port;
    int lismax;    // 监听队列大小
    int taskmax;   // 任务队列大小
    int threadnum; // 线程数目
};

// 前置声明
class Socket;

// 任务
typedef struct
{
    Socket *con;
} task_t;

// 线程池类
class ThreadPool
{
public:
    ThreadPool(int num, int task_num) : max_queue(task_num), thread_num(num)
    {
        queue = NULL;
        stop = 0;
    }

    bool Thread_Pool_Init();
    void Add_Task(Socket *csocket); // 向任务队列添加任务。
    void Start_Thread();
    void Work(); // 线程具体作的事情

private:
    task_t *queue; // 任务队列
    int front;     // 队头指针
    int rear;      // 队尾指针
    int count;     // 当前任务数量

    int stop; // 线程池是否退出

    int max_queue = 1024;
    int thread_num = 4; //

    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

// 基类 Socket
class Socket
{
public:
    Socket(int fd, int epfd) : m_fd(fd), m_epfd(epfd)
    {
    }

    ~Socket()
    {
        close(m_fd);
    }

    virtual void Handle_Data() = 0; // 纯虚函数 ,抽象类

    int m_fd;   // 套接子描述符
    int m_epfd; // 描述符属于那个epoll内核事件表（红黑树)
};

// 派生类，处理监听套接字的类
class LisSocket : public Socket
{
public:
    LisSocket(int fd, int epfd) : Socket(fd, epfd)
    {
    }

    void Handle_Data(); // 接受新连接 accept()
private:
    void ResetEvent();
};

// 派生类，处理连接套接字的类
class ConSocket : public Socket1
{
public:
    ConSocket(int fd, int epfd) : Socket(fd, epfd)
    {
    }
    void Handle_Data(); // 接受客户端发送的数据 recv
private:
    void ResetEvent();
    void Get_OpType(char buff[]); // 解析json报文,生成json对象

    void Send_Ok();
    void Send_Err();

    void User_Register();
    void User_Login();

    void Show_Ticket(); // 显示可预约的信息
    void Yd_Ticket();   // 预定

    void Show_My_Ticket();
    void Qx_Ticket();
    

private:
    Json::Value m_val; //
    int m_OpType;      // 操作类型
};

// 连接操作数据库的类
class MysqlClient
{
public:
    MysqlClient()
    {
        db_ip = "127.0.0.1";
        db_port = 3306;
        db_name = "2504db";
        dbuser_name = "root";
        db_passwd = "111111";
    }

    MysqlClient(string ip, short port, string dbname, string dbusername, string passwd)
    {
        db_ip = ip;
        db_port = port;
        db_name = dbname;
        dbuser_name = dbusername;
        db_passwd = passwd;
    }
    ~MysqlClient()
    {
        mysql_close(&mysql_con);
    }

    bool Connect_MysqlServer();
    bool Db_User_Register(const string &tel, const string &name, const string &passwd);
    bool Db_User_Login(const string &tel, string &name, const string &passwd);
    bool Db_Show_Ticket(Json::Value &res);
    bool Db_Yd_Ticket(const string &tel, const string &tk_id);
    bool Db_Show_My_Ticket(const string &tel,Json::Value &res);
    bool Db_Qx_Ticket(const string &yd_id);
    
    void Mysql_Begin();//开启事物
    void Mysql_RollBack();//回滚
    void Mysql_Commit();//提交事物

private:
    string db_ip;
    short db_port;
    string db_name;
    string dbuser_name;
    string db_passwd;
    MYSQL mysql_con;
};

class TcpSer
{
public:
    TcpSer(const SerConf &sc) : m_conf(sc), m_pool(sc.threadnum, sc.taskmax)
    {
        m_sockfd = -1;
        m_epfd = -1;
        m_num = -1;
    }

    bool Ser_Init();
    void Run();

private:
    bool create_socket(); // 创建监听套接子
    void do_event();      // 处理epoll返回的就绪事件
private:
    int m_sockfd;   // 监听套接子
    SerConf m_conf; // 配置文件的信息

    int m_epfd;                  // epoll 实例的文件描述符
    int m_num;                   // 本次检测到的就绪事件数量
    struct epoll_event m_evs[EV_MAX]; // 保存就绪事件的数组

    ThreadPool m_pool; // 线程池对象
};