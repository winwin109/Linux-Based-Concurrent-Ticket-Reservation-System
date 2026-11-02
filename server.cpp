#include "server.h"
bool SerConf::ReadConf(string filename) // 读取配置文件
{
    FILE *fp = fopen(filename.c_str(), "r");
    if (fp == NULL)
    {
        cout << "open file: " << filename << " err!" << endl;
        return false;
    }

    char buff[128] = {0};
    int num = 1; // 第几行
    while (fgets(buff, 127, fp))
    {
        if (strncmp(buff, "#", 1) == 0 || strncmp(buff, "\n", 1) == 0)
        {
            num++;
            continue;
        }

        if (buff[strlen(buff) - 1] == '\n')
        {
            buff[strlen(buff) - 1] = '\0';
        }

        char *k = strtok(buff, " ");
        char *v = strtok(NULL, " ");
        if (k == NULL || v == NULL)
        {
            cout << "配置文件my.cnf第: " << num++ << "行无法解析:" << buff << endl;
            continue;
        }

        if (strcmp(k, "ip") == 0)
        {
            ips = v;
        }
        else if (strcmp(k, "port") == 0)
        {
            port = atoi(v);
        }
        else if (strcmp(k, "lismax") == 0)
        {
            lismax = atoi(v);
        }
        else if (strcmp(k, "taskmax") == 0)
        {
            taskmax = atoi(v);
        }
        else if (strcmp(k, "threadnum") == 0)
        {
            threadnum = atoi(v);
        }
        else
        {
            cout << "配置文件第：" << num << "行,无法解析的k: " << k << endl;
        }

        num++;
    }
    return true;
}
void SerConf::PrintInfo() // 打印读取的配置信息
{
    cout << "----SerConf----" << endl;
    cout << "ip: " << ips << endl;
    cout << "port: " << port << endl;
    cout << "lismax: " << lismax << endl;
    cout << "taskmax: " << taskmax << endl;
    cout << "threadnum: " << threadnum << endl;
    cout << "----end----" << endl;
}

void *worker_thread(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    if (pool != nullptr)
    {
        pool->Work();
    }

    return NULL;
}

bool ThreadPool::Thread_Pool_Init()
{
    queue = (task_t *)malloc(sizeof(task_t) * max_queue);
    if (nullptr == queue)
    {
        return false;
    }

    front = 0;
    rear = 0;
    count = 0;

    stop = 0; // 不退出

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    return true;
}
void ThreadPool::Add_Task(Socket *csocket) // 向任务队列添加任务。
{
    pthread_mutex_lock(&mutex);
    if (count >= max_queue)
    {
        delete csocket;
        pthread_mutex_unlock(&mutex);
        return;
    }

    queue[rear].con = csocket;
    rear = (rear + 1) % max_queue;
    count++; // 任务数量加一

    pthread_cond_signal(&cond); // 唤醒一个线程，处理任务
    pthread_mutex_unlock(&mutex);
}
void ThreadPool::Start_Thread()
{
    pthread_t ids[thread_num];
    for (int i = 0; i < thread_num; i++)
    {
        pthread_create(&ids[i], NULL, worker_thread, this);
    }
}
void ThreadPool::Work() // 线程具体作的事情  从任务队列获取任务
{
    while (true)
    {
        pthread_mutex_lock(&mutex);
        while (count == 0 && !stop)
        {
            pthread_cond_wait(&cond, &mutex); // 当前线程放入条件便利等待队列
        }

        if (stop) // 退出线程
        {
            break;
        }

        Socket *con = queue[front].con;
        front = (front + 1) % max_queue;
        count--;

        pthread_mutex_unlock(&mutex);

        if (con != nullptr)
        {
            con->Handle_Data();
        }
    }
}

// LisSocket
void LisSocket::Handle_Data()
{
    // 测试
    int c = accept(m_fd, NULL, NULL);
    if (c < 0)
    {
        return;
    }

    ResetEvent(); // 重置sockfd上的读事件,内核就可以继续检测了

    cout << "accept c=" << c << endl;
    ConSocket *cs = new ConSocket(c, m_epfd);
    if (nullptr == cs)
    {
        close(c);
        return;
    }

    struct epoll_event ev;
    ev.data.ptr = cs;
    ev.events = EPOLLIN | EPOLLONESHOT; // 只触发一次，等工作线程处理完事件，再重置

    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, c, &ev) == -1)
    {
        cout << "epoll ctl add c err" << endl;
        return;
    }
}

void LisSocket::ResetEvent() // 重置读事件 -- sockfd,
{
    struct epoll_event ev;
    ev.data.ptr = this; // 处理这个读事件的对象
    ev.events = EPOLLIN | EPOLLONESHOT;
    if (epoll_ctl(m_epfd, EPOLL_CTL_MOD, m_fd, &ev) == -1)
    {
        cout << "Reset Event err" << endl;
    }
}

// ConSocket
void ConSocket::Get_OpType(char buff[])
{
    Json::Reader Read;
    m_val.clear();
    if (!Read.parse(buff, m_val))
    {
        cout << "josn 解析失败:";
        cout << buff << endl;
        m_OpType = -1;
        return;
    }

    m_OpType = m_val["type"].asInt();
}

void ConSocket::Send_Ok()
{
    Json::Value v;
    v["status"] = "OK";
    send(m_fd, v.toStyledString().c_str(), strlen(v.toStyledString().c_str()), 0);
}

void ConSocket::Send_Err()
{
    Json::Value v;
    v["status"] = "Err";
    send(m_fd, v.toStyledString().c_str(), strlen(v.toStyledString().c_str()), 0);
}

void ConSocket::User_Register()
{
    string username = m_val["user_name"].asString();
    string usertel = m_val["user_tel"].asString();
    string userpasswd = m_val["user_passwd"].asString();

    cout << username << endl;
    cout << usertel << endl;
    cout << userpasswd << endl;

    // 访问 数据库
    MysqlClient mycli;
    if (!mycli.Connect_MysqlServer())
    {
        Send_Err();
        return;
    }

    if (!mycli.Db_User_Register(usertel, username, userpasswd))
    {
        Send_Err();
        return;
    }

    Send_Ok();
    return;
}

void ConSocket::User_Login()
{
    string usertel = m_val["user_tel"].asString();
    string passwd = m_val["user_passwd"].asString();
    string username;

    MysqlClient cli;
    if (!cli.Connect_MysqlServer())
    {
        Send_Err();
        return;
    }

    if (!cli.Db_User_Login(usertel, username, passwd))
    {
        Send_Err();
        return;
    }

    Json::Value ra;
    ra["status"] = "OK";
    ra["user_name"] = username;
    send(m_fd, ra.toStyledString().c_str(), strlen(ra.toStyledString().c_str()), 0);
}
void ConSocket::Show_Ticket() // 显示可预约的信息
{
    Json::Value res;
    MysqlClient cli;
    if (!cli.Connect_MysqlServer())
    {
        Send_Err();
        return;
    }

    if (!cli.Db_Show_Ticket(res))
    {
        Send_Err();
        return;
    }

    send(m_fd, res.toStyledString().c_str(), strlen(res.toStyledString().c_str()), 0);
}
void ConSocket::Yd_Ticket() // 预定
{
    string user_tel = m_val["user_tel"].asString();
    string tk_id = m_val["ticket_id"].asString();

    MysqlClient cli;
    if (!cli.Connect_MysqlServer())
    {
        Send_Err();
        return;
    }

    if (!cli.Db_Yd_Ticket(user_tel, tk_id))
    {
        Send_Err();
        return;
    }

    Send_Ok();
    return;
}
void ConSocket::Show_My_Ticket()
{
    string tel = m_val["user_tel"].asString();
    MysqlClient mysql;

    if (!mysql.Connect_MysqlServer())
    {
        cout << "mysql connect error" << endl;
        return;
    }

    Json::Value res;

    if (!mysql.Db_Show_My_Ticket(tel, res))
    {
        res["status"] = "ERR";
    }

    string str = res.toStyledString();

    send(m_fd,str.c_str(),strlen(str.c_str()),0);
}
void ConSocket::Qx_Ticket()
{
    string yd_id = m_val["yd_id"].asString();

    MysqlClient mysql;

    if (!mysql.Connect_MysqlServer())
    {
        return;
    }

    bool res = mysql.Db_Qx_Ticket(yd_id);

    Json::Value val;

    if (res)
    {
        val["status"] = "OK";
    }
    else
    {
        val["status"] = "ERR";
    }

    string str = val.toStyledString();

    send(m_fd,str.c_str(),strlen(str.c_str()),0);
}
void ConSocket::Handle_Data()
{
    char buff[1024] = {0};
    int n = recv(m_fd, buff, 1023, 0);
    if (n <= 0)
    {
        cout << "client close" << endl;
        delete this;
        return;
    }

    Get_OpType(buff);

    switch (m_OpType)
    {
    case REGISTER:
        User_Register();
        break;
    case LOGIN:
        User_Login();
        break;
    case SHOW:
        Show_Ticket();
        break;
    case YD:
        Yd_Ticket();
        break;
    case XSYD:
        Show_My_Ticket();
        break;
    case QXYD:
        Qx_Ticket();
        break;
    default:
        break;
    }
    ResetEvent(); // 重置读事件，让epoll继续检测读事件
}

void ConSocket::ResetEvent() // 重置读事件 -- c
{
    struct epoll_event ev;
    ev.data.ptr = this;
    ev.events = EPOLLIN | EPOLLONESHOT;
    if (epoll_ctl(m_epfd, EPOLL_CTL_MOD, m_fd, &ev) == -1)
    {
        cout << "Reset event c err" << endl;
    }
}

// mysqlclient
bool MysqlClient::Connect_MysqlServer()
{
    // 初始化连接句柄
    if (mysql_init(&mysql_con) == NULL)
    {
        return false;
    }
    // 连接数据库
    if (mysql_real_connect(&mysql_con, db_ip.c_str(), dbuser_name.c_str(), db_passwd.c_str(), db_name.c_str(), db_port, NULL, 0) == NULL)
    {
        return false;
    }

    return true;
}
bool MysqlClient::Db_User_Register(const string &tel, const string &name, const string &passwd)
{
    // insert into user_info values(0,'13500000001','小李','111111',1,curdate())
    string sql = string("insert into user_info values(0,'") + tel + string("','") + name + string("','") + passwd + string("',1,curdate())");
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        return false;
    }

    return true;
}
bool MysqlClient::Db_User_Login(const string &tel, string &name, const string &passwd)
{
    // select user_name from user_info where user_tel=13700000001 and user_passwd='123456';
    string sql = string("select user_name from user_info where user_tel=") + tel + string(" and user_passwd=") + passwd;
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        return false;
    }

    // 获取结果集
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        return false;
    }

    // 获取结果中的行数,(实际应该只有一行)
    int num = mysql_num_rows(r);
    if (num != 1)
    {
        mysql_free_result(r);
        return false;
    }

    // 获取一行记录(只有一行，一列，就是用户名)
    MYSQL_ROW row = mysql_fetch_row(r);
    name = row[0];
    mysql_free_result(r);
    return true;
}

bool MysqlClient::Db_Show_Ticket(Json::Value &res)
{
    // select * from ticket_table
    string sql = "select * from ticket_table";
    if (mysql_query(&mysql_con, sql.c_str()) != 0) // 执行sql语句，查询票的信息
    {
        return false;
    }

    MYSQL_RES *r = mysql_store_result(&mysql_con); // 获取查询的结果集
    if (r == nullptr)
    {
        return false;
    }

    int num = mysql_num_rows(r); // 获取结果集有多少条记录
    res["status"] = "OK";
    res["num"] = num;

    if (num == 0)
    {
        mysql_free_result(r); // 释放结果集占用的空间
        return true;
    }

    for (int i = 0; i < num; i++)
    {
        Json::Value tmp;
        MYSQL_ROW row = mysql_fetch_row(r);
        // tk_id | tk_name         | tk_max | count | datetime
        tmp["ticket_id"] = row[0];
        tmp["ticket_name"] = row[1];
        tmp["ticket_max"] = row[2];
        tmp["ticket_count"] = row[3];
        tmp["day_time"] = row[4];

        res["ticket_arr"].append(tmp);
    }

    mysql_free_result(r);
    return true;
}

bool MysqlClient::Db_Yd_Ticket(const string &tel, const string &tk_id)
{
    Mysql_Begin(); // 开启事务
    // select count,tk_max from ticket_table where tk_id=2;
    string sql_count_max = string("select count,tk_max from ticket_table where tk_id=") + tk_id+" for update";
    if (mysql_query(&mysql_con, sql_count_max.c_str()) != 0)
    {
        return false;
    }

    MYSQL_RES *r = mysql_store_result(&mysql_con); // 获取结果集
    if (r == nullptr)
    {
        return false;
    }

    int num = mysql_num_rows(r); // 获取记录
    if (num == 0)
    {
        mysql_free_result(r);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(r);
    int count = atoi(row[0]);
    int max = atoi(row[1]);

    mysql_free_result(r);

    if (count >= max) // 没有票
    {
        return false;
    }

    count++;

    
    // update  ticket_table set count=1  where tk_id = 2;
    string sql_setcount = string("update  ticket_table set count=") + to_string(count) + string(" where tk_id =") + tk_id;
    if (mysql_query(&mysql_con, sql_setcount.c_str()) != 0)
    {
        Mysql_RollBack();
        return false;
    }

    // insert into yd_table values(0,13700000001,2,now());
    string sql_yd = string("insert into yd_table values(0,") + tel + string(",") + tk_id + string(",now())");
    if (mysql_query(&mysql_con, sql_yd.c_str()) != 0)
    {
        Mysql_RollBack();
        return false;
    }

    Mysql_Commit();
    return true;
}

bool MysqlClient::Db_Show_My_Ticket(const string &tel, Json::Value &res)
{
    string sql = string("select * from yd_table where tel=") + tel;

    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << mysql_error(&mysql_con) << endl;
        return false;
    }

    MYSQL_RES *r = mysql_store_result(&mysql_con);

    if (r == nullptr)
    {
        return false;
    }

    int num = mysql_num_rows(r);

    res["status"] = "OK";
    res["num"] = num;

    for (int i = 0; i < num; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(r);

        res["yd_arr"][i]["yd_id"] = row[0];
        res["yd_arr"][i]["tel"] = row[1];
        res["yd_arr"][i]["ticket_id"] = row[2];
        res["yd_arr"][i]["ctime"] = row[3];
    }

    mysql_free_result(r);

    return true;
}
bool MysqlClient::Db_Qx_Ticket(const string &yd_id)
{
    Mysql_Begin();

    // 1. 查询预约对应的票id
    string sql_get_tkid =
        string("select tk_id from yd_table where yd_id=") + yd_id+" for update";

    if (mysql_query(&mysql_con, sql_get_tkid.c_str()) != 0)
    {
        Mysql_RollBack();
        return false;
    }

    MYSQL_RES *r = mysql_store_result(&mysql_con);

    if (r == nullptr)
    {
        Mysql_RollBack();
        return false;
    }

    int num = mysql_num_rows(r);

    if (num == 0)
    {
        mysql_free_result(r);
        Mysql_RollBack();
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(r);

    int tk_id = atoi(row[0]);

    mysql_free_result(r);

    // 2. 删除预约记录
    string sql_delete =
        string("delete from yd_table where yd_id=") + yd_id;

    if (mysql_query(&mysql_con, sql_delete.c_str()) != 0)
    {
        Mysql_RollBack();
        return false;
    }

    // 3. 恢复票数
    string sql_update =
        string("update ticket_table set count=count-1 where tk_id=") + to_string(tk_id);

    if (mysql_query(&mysql_con, sql_update.c_str()) != 0)
    {
        Mysql_RollBack();
        return false;
    }

    // 4. 提交事务
    Mysql_Commit();

    return true;
}
void MysqlClient::Mysql_Begin() // 开启事务
{
    if (mysql_query(&mysql_con, "begin") != 0)
    {
        cout << "启动事务失败" << endl;
    }
}
void MysqlClient::Mysql_RollBack() // 回滚
{
    if (mysql_query(&mysql_con, "rollback") != 0)
    {
        cout << "回滚事务失败" << endl;
    }
}
void MysqlClient::Mysql_Commit() // 提交事务
{
    if (mysql_query(&mysql_con, "commit") != 0)
    {
        cout << "提交事务失败" << endl;
    }
}
// tcpser类
bool TcpSer::Ser_Init()
{
    if (!create_socket()) // 创建监听套接子
    {
        return false;
    }

    m_epfd = epoll_create1(0); // 创建内核事件表 epoll,红黑树(添加描述符），就绪队列(就绪)
    if (-1 == m_epfd)
    {
        return false;
    }

    LisSocket *sock = new LisSocket(m_sockfd, m_epfd);
    if (nullptr == sock) // NULL 0
    {
        cout << "create LisSocket err" << endl;
        return false;
    }
    struct epoll_event ev;
    // ev.data.fd = m_sockfd; //旧版学习使用
    ev.data.ptr = sock;                 // 处理监听套接子的对象的指针
    ev.events = EPOLLIN | EPOLLONESHOT; // 只检测一次

    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_sockfd, &ev) == -1) // 将套接子描述添加到内核事件表(红黑树)
    {
        cout << "epoll ctl add err" << endl;
        return false;
    }

    // 线程池，任务队列
    m_pool.Thread_Pool_Init(); // 线程池类初始化
    m_pool.Start_Thread();
    return true;
}
void TcpSer::do_event()
{
    for (int i = 0; i < m_num; i++)
    {
        if (m_evs[i].events & EPOLLIN)
        {
            m_pool.Add_Task((Socket *)m_evs[i].data.ptr); // 就绪事件入队列
        }
    }
}
void TcpSer::Run()
{
    while (true)
    {
        m_num = epoll_wait(m_epfd, m_evs, EV_MAX, 5000); //-1永久阻塞直到事件就绪或者失败
        if (-1 == m_num)
        {
            cout << "epoll_wait err" << endl;
        }
        else if (0 == m_num)
        {
            cout << "time out" << endl;
        }
        else
        {
            do_event();
        }
    }
}
bool TcpSer::create_socket()
{
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == m_sockfd)
    {
        return false;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(m_conf.port);
    saddr.sin_addr.s_addr = inet_addr(m_conf.ips.c_str());
    if (bind(m_sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        return false;
    }

    if (listen(m_sockfd, m_conf.lismax) == -1)
    {
        return false;
    }

    return true;
}
int main()
{
    SerConf conf;
    if (!conf.ReadConf("my.cnf"))
    {
        exit(1);
    }

    conf.PrintInfo();

    // tcpser
    TcpSer ser(conf);
    if (!ser.Ser_Init())
    {
        cout << "Ser Init failed" << endl;
        exit(1);
    }

    ser.Run();

    exit(0);
}