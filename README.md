# 服务器端：

## 概要：即`main`函数

1.定义一个`Serconf`类对象->调用函数读取配置文件里的信息->打印出来

2.定义一个`TcpSer`类对象`ser`->调用函数初始化`ser`->调用函数运行`ser`

## 配置

### `1.SerConf`结构体

1.构造函数：先给结构体内的成员变量一个默认值

2.==成员变量==：`ip`类型，端口号，监听队列大小，任务队列大小，线程数目

3.成员函数：读取配置文件，打印读取的配置信息

### 2.读取配置文件（`ReadConf()`)

#### ==1）概要==

这个函数的主要目的就是把配置文件的数据读取进来

#### 2）具体操作

0.返回值为bool类型，参数为文件名

1.只读方式打开文件

2.`while`循环遍历文件的每一行，先放入`buff`缓存数组中，每一行处理完后，用`num`变量记录行号

3.比较每一行的一个字符，如果遇到注释符或者换行符，代表这行是注释或者空，直接跳过即可

4.由于`fgets`会把换行符也给缓存数组，会打扰后续的比较，因此把换行符修改成`'\0'`

5.切割得到每一行第一个字符（`strtok`函数，遇空停止），即得到键；接着切割一个字符，即得到值

6.如果键或者值有一个为空，则表示这行无法解析，直接跳转下一行

7.比较每一行键是否与预期相同（例如第一行是不是`ip`)（用`strcmp`函数），如果相同，则把配置文件里解析出的值赋值给相对应的成员变量；如果不相同，打印出行号无法解析的信息，跳转下一行

##  ==初始化==

### `1.Tcpser`类

1.构造函数：

参数传入一个`SerConf`对象，除了`m_conf`,`m_pool`两个成员变量，其他几个成员变量（除了数组都赋值为-1）

2.成员函数：

初始化（`Ser_Init();`），运行（`Run();`），创建监听套接字（`create_socket(); `），处理`epoll`返回的就绪事件（`do_event();`）

3.成员变量：

监听套接字，配置文件的信息，`epoll`实例的文件描述符，本次检测到的就绪事件数量，保存就绪事件的数组（`struct epoll_event`），线程池对象

4.用到的内置结构体:

##### `struct epoll_event`

~~~
struct epoll_event {
    uint32_t     events;    // 事件掩码（位图）
    epoll_data_t data;      // 用户数据（联合体）
};

typedef union epoll_data {
    void    *ptr;    // 自定义指针（常用）
    int      fd;     // 关联的文件描述符（常用）
    uint32_t u32;    // 32位整数
    uint64_t u64;    // 64位整数
} epoll_data_t;
~~~

### `2.Ser_Init()`

#### ==1）概要：==

这个函数准备好了：内核事件表，监听套接字，线程池

#### 2）具体操作：

1.创建监听套接字（`create_socket()`)

2.创建`epoll`实例（`epoll_create1()`）

3.new一个`LisSocket`类对象

4.定义一个`epoll_event`结构体对象，将监听套接字的对象指针给结构体对象，将EPPOLLIN和EPOLLONESHOT事件给结构体对象（检测可读事件且只检测一次，防止多个线程来处理同一个事件，保证了线程安全）

5.将监听套接字的文件描述符添加到内核事件表（`epoll_ctl()`)



6.初始化线程池（`Thread_Pool_Init（）`）

7.启动线程池（`Start_Thread()`）

#### 3）API

##### `epoll_createl()`

1 创建`epoll`实例的系统调用，会返回一个`epoll`的文件描述符

~~~
int epoll_create1(int flags);//一般都是0
~~~

##### `epoll_ctl()`

2 对`epoll`实例进行操作（添加、修改或者删除）：

~~~
int epoll_ctl(int epfd，int op，int fd，struct epoll_event * event);
1) int epfd： epoll_create()函数返回的epoll实例的句柄。
2) int op： 需要执行的操作，添加，修改，删除。
3) int fd： 需要添加，修改，删除的socket文件描述符
4) struct epoll_event * event： 需要epoll监视的fd对应的事件类 型
~~~

### 3.`Create_Socket()`

#### ==1）概要:==

创建一个可以被动监听客户端的监听套接字

#### 2）具体操作

1.创建一个套接字（`socket`)

2.创建一个套接字地址结构对象

3.先用0初始化结构体，清洗结构体中可能的脏数据

4.将配置信息导入地址对象（注意：端口号和地址需要用函数将原本的字符串转变成网络通信接收的格式）

5.将套接字与用于通信的地址和端口号绑定（`bind()`）

6.将套接字转变为监听文件描述符（`listen()`）

#### 3）API

##### `socket()`

1 创建套接字：

~~~
int socket(int domain, int type, int protocol);
domain:指定协议族（IPV4）
type：选择面向连接的流式套接字（TCP），无连接的数据报套接字（UDP）
protocol:协议（一般都传0，让系统根据type自己选）
~~~

##### `struct socket_in`

2 套接字地址结构：

~~~
struct sockaddr_in
{
	sa_family_t		sin_family;//地址族
	uint16_t		sin_port;//16位TCP/UDP端口号
	struct in_addr	sin_addr;//32位IP地址
	char			sin_zero[8];//不使用
}
struct in_addr
{
	In_addr_t		a_addr;//32位IPv4地址
}
~~~

##### `bind()`

3 把用于通信的地址和端口号绑定给`socket`：

~~~
int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
（1）参数 sockfd ，需要绑定的socket。
（2）参数 addr ，存放了服务端用于通信的地址和端口。
（3）参数 addrlen ，表示 addr 结构体的大小
（4）返回值：成功则返回0 ，失败返回-1，错误原因存于 errno 中
~~~

##### `listen()`

4 将套接字转变为被动监听客户端的监听文件描述符：

~~~
int listen(int sockfd, int backlog);
(1)sockfd 表示socket创建的套接字文件描述符
(2)backlog 指定队列的容量
这个队列用于记录正在连接但是还没有连接完成的客户端，一般设置队列的容量为2，3即可。队列的最大容量需要小于30
~~~

### 4.`Socket`基类

1.构造函数

2.析构函数：关闭当前套接字文件描述符

3.纯虚函数`Handle_Data()`，等待后面被派生类继承

4.成员变量：套接字描述符，`epoll`内核事件表的文件描述符

### 5.`LisSocket`类

0.继承`Socket`基类

1.构造函数：

2.成员函数：

`Handle_Data()`，公有，处理新连接

`ResetEven`，私有，重置事件状态

### 6.线程池类`ThreadPool`类

1.构造函数，参数为线程数和任务数量上限，队列初始化为空，退出变量初始化为0

2.成员函数：

` bool Thread_Pool_Init();` //线程池初始化

``void Add_Task(Socket *csocket);` // 向任务队列添加任务。

`void Start_Thread();`//启动线程

``void Work();` // 线程具体作的事情

3.成员变量：

  `task_t *queue; `// 任务队列

~~~
typedef struct
{
    Socket *con;
} task_t;//任务结构体
~~~

  `int front;`   // 队头指针

  `int rear;`    // 队尾指针

  `int count;`   // 当前任务数量

  `int stop;` // 线程池是否退出

  `int max_queue = 1024;`//队列最多容纳任务数量

  `int thread_num = 4; `//线程数量

  `pthread_mutex_t mutex；`//互斥锁

  `pthread_cond_t cond;`//条件变量

### 7.`Thread_Pool_Init()`

#### 1.概要

这个函数准备好了线程池要用到的任务队列，互斥锁，条件变量

#### 2.具体操作

1.创建一个任务队列，申请内存

2.队头指针，队尾指针，当前任务数量全部初始化为0，当前线程池初始化为不退出

3.锁，条件变量初始化

#### 3.API

##### ` pthread_mutex_init()`

1 互斥锁初始化

~~~
#include <pthread.h>
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);

mutex参数指向要初始化的互斥锁，attr参数指定互斥锁的属性。如果attr为NULL，则使用默认的互斥锁属性。
~~~

##### `pthread_cond_init()`

2 条件变量初始化

~~~
#include <pthread.h>
int pthread_cond_init（pthread_cond_t *cond， const pthread_condattr_t *attr）;

cond参数指向要初始化的条件变量，attr参数指定条件变量的属性。如果attr为NULL，则使用默认的条件变量属性。
~~~

### 8.`Start_Thread()`

#### 1.概要

这个函数为线程池创建了固定数量的线程。

#### 2.具体操作

`pthread_t`就是`unsigned long int`

1.创建一个`pthread_t`类型的数组

2.循环创建线程

#### 3.API

##### `pthread_create()`

~~~
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
                 
thread： 指向线程ID存储位置的指针。
attr： 线程属性。一般为NULL
start_routine：新建的线程从此函数开始运行
arg： 指向传递给线程函数的参数的指针。
~~~

### 9.`worker_thread()`

#### 1.概要：

强转线程类型为`ThreadPool`,为后续线程工作做准备

#### 2.具体操作：

1.强转线程类型为`ThreadPool`

2.如果线程不为空，就调用`Work`函数

## ==运行==

### 1.`Run()`

#### 1.概要

得到就绪事件表

#### 2.具体操作

1.`while`无限循环

2.`epoll_wait()`得到就绪事件表和数量

3.如果不是超时或者错误，就进`do_event()`

#### 3.API

##### `epoll_wait():`

等待监听的所有`fd`相应事件的发生：

~~~
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);

1.epfd:epoll实例的句柄
2.events:epoll监测到符合要求的事件集合从内核复制到该数组中
3.maxevents：数组中最多存放的事件数量
4.timeout:如果一直监测不到事件，等多久退出
5.返回值：成功就返回事件数量，超时返回0，错误返回-1
~~~

###  2.`do_event()`

#### 1.概要

就绪事件入队列（Part1)

#### 2.具体操作

1.遍历就绪事件数组

2.如果这个事件存在且可读，调用函数放入队列（`Add_Task()`）

### 3.生产者：`Add_Task()`

#### 1.概要

向任务队列添加任务，并唤醒线程

#### 2.具体操作

0.参数：`Socket`类型指针

1.上锁

2.如果当前任务数量大于等于队列最大值，删掉，取锁，返回空

3.把任务放进队列，`rear`+1，任务数量+1

4.唤醒一个线程，处理任务

5.取锁

#### 3.API

##### `pthread_cond_signal()`

唤醒一个线程，处理任务

~~~
pthread_cond_signal(&cond)//唤醒一个线程
~~~

### 4.消费者：`Work()`

#### 1.概要

线程从任务队列获取任务

#### 2.具体操作

1.`while`死循环

2.加锁

3.如果任务队列为空且线程池不停止（停止就退出），就把当前线程放入条件变量等待队列，等待被唤醒

4.从任务队列取出任务，修改任务队列公共资源有关变量的值

5.取锁

6.如果取出来任务就转交给处理数据(`Handle_Data()`)

#### 3.API

##### `pthread_mutex_lock()`

1.上锁

~~~
pthread_mutex_lock(&mutex);
~~~

##### `pthread_mutex_unlock()`

2.开锁

~~~
pthread_mutex_unlock(&mutex);
~~~

##### `pthread_cond_wait()`

3.条件变量等待

~~~
pthread_cond_wait(&cond, &mutex)

pthread_cond_wait()函数一进入wait状态就会自动release mutex，线程被唤醒之后又自动获锁。他只负责等待和通知
~~~

### 5.`LisSocket::Handle_Data()`

#### 1.概要

把新连接的客户端的文件描述符给`epoll`监听

#### 2.具体操作

1.`accept()`,与客户端连接，返回值是一个新可用的连接套接字，如果小于0直接返回空

2.重置`sockfd`上的读事件，内核就可以继续检测了（`ResetEvent()`)

3.创建一个`ConSocket()`对象

4.创建一个`struct epoll_event`，把`ConSocket`对象的地址和事件导入

5.把`accept()`得到的连接套接字给`epoll()`监听（`epoll_ctl()`)

#### 3.API

##### `accept()`

默认会阻塞进程，直到有一个客户连接建立后返回，它返回的是一个新可用的套接字，这个套接字是连接套接字。

~~~
#include<sys/socket.h>
int accept(int sockfd, struct sockaddr* addr, socklen_t* len)
返回：非负描述字——成功， -1——失败
//accept(m_fd, NULL, NULL);

1.sockfd:监听套接字
2.addr:指定客户端的地址，如果对客户端的地址不感兴趣填NULL
3.len:指明addr结构所占字节数，也可以填NULL
~~~

### 6.`ResetEvent()`

#### 1.概要

重置读时间

#### 2.具体操作

1.创建一个`epoll_event`结构体，重新把当前文件设置成`EPOLLIN\EPOLLONESHOT`

2.将当前就绪事件的`fd`添加给`epoll_ctl`

### 7.`ConSocket`类

0.继承`Socket`类

1.构造函数，参数传入`fd`,`epfd`

2.成员函数：

公有：

` void Handle_Data();`// 接受客户端发送的数据`recv`

私有：

`void ResetEvent();`//重置事件

`void Get_OpType(char buff[]);` // 解析`json`报文,生成`json`对象

`void Send_Ok();`    `void Send_Err();`

`void User_Register();`   `void User_Login();`   `void Show_Ticket();` // 显示可预约的信息

  `void Yd_Ticket();`  // 预定    `void Show_My_Ticket();`    `void Qx_Ticket();`

3.成员变量：

`Json::Value m_val; `：`Json`数据的容器

`int m_OpType;`：操作类型

### 8.`ConSocket::Handle_Data()`

#### 1.概要：

得到客户端信息，确认做什么业务

#### 2.具体操作：

1.创建一个缓存区数组

2.接收客户端信息（`recv()`)

3.得到用户选的信息（`Get_OpType()`）

4.用`switch`语句，确定客户端需要的业务函数

5.上一次事件已经得到处理了，重置读事件，让`epoll`继续检测

#### 3.API:

##### `recv()`

~~~
int recv( SOCKET s, char *buf, int len, int flags);

1.s是接收端的套接字描述符
2.buff是接收缓存区
3.len是buff的长度
4.一般置为0
~~~

### 9.`Get_OpType()`

#### 1.概要

把服务器端传来的字符串解析转换成`Json::Value`对象

#### 2.具体操作

1.创建一个`Json::Reader`对象

2.把`Json::Value`对象`m_val`清空，防止脏数据干扰

3.解析接收缓存区（`Read.parse()`)

4.把解析得到的`“type"`给`m_OpType`

#### 3.API

##### `Json::Reader`

用于将JSON字符串或文件解析为`Json::Value`对象

~~~
Read.parse(buff, m_val)
~~~

## 业务

### 1.`User_Register()`

#### 1.概要

注册新用户，同步更新数据库

#### 2.具体操作

1.把`Json`对象里的`username``usertel``userpasswd`转变成字符串

2.创建数据库对象

3.连接主机数据库(`Connect_MysqlServer()`)

4.把注册（插入新数据）信息写入数据库(`Db_User_Register()`)

5.以上操作如果失败返回err，成功返回ok

### 2.`User_Login()`

#### 1.概要

得到用户名，发给客户端

#### 2.具体操作

1.从`Json`容器中把电话和密码取出来

2.创建数据库对象，连接数据库

3.操作数据库，得到用户名

4.创建`Json`对象，`status`赋值为`ok`,`user_name`赋值为用户名

5.把`Json`数据包发送到发送缓冲区

#### 3.API

##### `send()`

不论是客户还是服务器 应用程序都用send函数来向TCP连接的另一端发送数据。

~~~
int send( SOCKET s,char *buf,int len,int flags );

参数一：指定发送端套接字 描述符；
参数二：存放应用程序要发送数据的缓冲区；
参数三：实际要发送的数据的字节数；
参数四：一般置为0。
~~~

### 3.`Show_Ticket()`

#### 1.概要

把从数据库得到的表信息发给发送缓冲区

#### 2.具体操作

1.创建`Json`容器对象，创建操作数据库的对象

2.连接数据库

3.从数据库拿表

4.把得到的`Json`数据集发送给发送缓冲区

### 4.`Yd_Ticket()`

#### 1.概要

连接数据库，完成票务预订并返回成功

#### 2.具体操作

1.从`Json`容器得到电话和票的`id`

2.创建一个数据库操作对象

3.连接数据库

4.操作数据库，完成预定有关的票务信息调整

5.发送一个`ok`

### 5.`Show_My_Ticket()`

同`Show_Ticket()`，只是查询不同的表

### 6.Qx_Ticket()

### 1.概要

操作数据库取消订单，把最后得到`Json`发给缓冲区

### 2.具体操作

1.得到预定记录的id

2.创建数据库连接对象

3.连接数据库

4.数据库操作取消订单，返回bool值

5.创建`Json`容器，如果数据库操作的结果为真，就把status设置为ok,反之为err

6.把`Json`数据集发到缓冲区

## 数据库

### 1.`MysqlClient`类

0.构造函数：缺省构造（直接给相关信息）

​					有参（通过参数赋值）

​	析构函数：调用`mysql_close()`函数，关闭连接

1.成员函数：

`bool Connect_MysqlServer();`

`bool Db_User_Register(const string &tel, const string &name, const string &passwd);`

`bool Db_User_Login(const string &tel, string &name, const string &passwd);`

`bool Db_Show_Ticket(Json::Value &res);`

`bool Db_Yd_Ticket(const string &tel, const string &tk_id);`

`bool Db_Show_My_Ticket(const string &tel,Json::Value &res);`

`bool Db_Qx_Ticket(const string &yd_id);`

`void Mysql_Begin();//开启事物`

`void Mysql_RollBack();//回滚`

`void Mysql_Commit();//提交事物`

2.成员属性：

`string db_ip;`  `short db_port;`  `string db_name;`  `string dbuser_name;`

`string db_passwd; ` `MYSQL mysql_con;`

3.API

##### `mysql_close()`

~~~
mysql_close(link_identifier)

关闭非持久的 MySQL 连接。
~~~

### 2.`Connect_MysqlServer()`

#### 1.概要:

连接主机数据库

#### 2.具体操作：

1.初始化数据库对象

2.与主机上的`MySql`数据库引擎建立连接

#### 3.API

##### `mysql_init()`

~~~
MYSQL *mysql_init(MYSQL *mysql) 

如果mysql是NULL指针，该函数将分配、初始化、并返回新对象。
否则，将初始化对象，并返回对象的地址。
~~~

##### `mysql_real_connect()`

尝试与运行在主机上的MySQL数据库引擎建立连接。

~~~
MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long client_flag) 

mysql：前面定义的MYSQL变量；
host：MYSQL服务器的地址；
user:登录用户名
passwd:登录密码
db:要连接的数据库
port:MYSQL服务器的TCP服务端口
unix_socket:取NULL
clientflag:一般取0
~~~

### 3.`Db_User_Register()`

#### 1.概要

拼接插入语句，并执行给数据库

#### 2.具体操作

1.拼接数据库语句（字符串）

2.调用函数执行语句（字符串转换成字符数组指针）

#### 3.API

##### `mysql_query()`

执行一句数据库语句

~~~
int mysql_query(MYSQL *mysql, const char *query) 
//query为执行的SQL语句对应的字符长串
//mysql:数据库对象
~~~

### 4.`Send_Err()`

#### 1.概要

发送一个`err`到发送缓冲区

#### 2.具体操作

1.创建一个`Json：：Value`类型变量

2.将变量的`status`键值赋为`ok`

3.调用函数发送到发送缓冲区（`send()`）

#### 3.API

##### `send()`

当调用该函数时，`send`先比较待发送数据的长度`len`和套接字`s`的发送缓冲的长度（因为待发送数据是要`copy`到套接字`s`的发送缓冲区的，注意并不是`send`把`s`的发送缓冲中的数据传到连接的另一端的，而是协议传的，`send`仅仅是把`buf`中的数据`copy`到`s`的发送缓冲区的剩余空间里

~~~
int send( SOCKET s,char *buf,int len,int flags );

//s:指定发送端套接字文件描述符
//buf:缓冲区
//len:要发送的字节数
//flags：一般为0
~~~

### 5.`Send_Ok()`

 #### 1.概要

发送一个`ok`到发送缓冲区

#### 2.具体操作：

同`Send_Err()`

### 6.`Db_User_Login()`

#### 1.概要

在数据库根据密码电话查询并取出用户名

#### 2.具体操作

1.组装数据库查询语句（查用户名）

2.执行这句语句

3.获取结果集

4.获取结果集的行数（只有1行）

5.获取第一行的记录，把第一个字符取出来，即为用户名

6.释放结果集内存

#### 3.API

##### `mysql_store_result()`

获取结果集

~~~
MYSQL_RES *
mysql_store_result(MYSQL *mysql)

在查询操作结束后，调用该函数，返回指向MYSQL_RES结构的指针。 如果查询未产生结果集（例如插入），或发生错误，则返回NULL。
~~~

##### `mysql_num_rows()`

获取结果集的行数

~~~
uint64_t
mysql_num_rows(MYSQL_RES *result)
~~~

##### `mysql_fetch_row()`

检索结果集的下一行

~~~
MYSQL_ROW
mysql_fetch_row(MYSQL_RES *result)
~~~

##### `mysql_free_result()`

释放结果集的内存

~~~
void
mysql_free_result(MYSQL_RES *result)
~~~

### 7.`Db_Show_Ticket()`

#### 1.概要

从数据库获得表所有内容

#### 2.具体操作

1.给出搜索全表的语句字符串

2.执行`sql`语句（`mysql_query()`)

3.获取查询的结果集（`mysql_store_result()`)

4.获取结果集行数（`mysql_num_rows()`)

5.把结果集里的状态改成`ok`,`num`改成得到的行数

6.如果行数为0，直接释放结果集占用的空间（`mysql_free_result()`)

7.循环遍历每一行，得到每一行的数据（`mysql_fetch_row()`),根据表结构，把数据放入`Json`数据集，把该条数据放进总结果集的`Json`数据集（数组形式）

8.释放结果集占用的空间

### 8.`Db_Yd_Ticket()`

#### 1.概要

操作数据库，完成预定有关的票务信息调整

#### 2.具体操作

1.开启事务（`Mysql_Begin()`)

2.拼接`sql`语句的字符串，先查询想买票的数量和上限

3.执行语句,失败返回false

4.获取结果集（`mysql_store_result()`)

5.获取结果行数，如果为0，直接释放结果集内存(`mysql_free_rows()`)

6.获取结果集第一行内容(`mysql_fetch_roe()`)，前两个字符分别是`count,max`

7.如果已卖的大于等于上限，说明没票了，直接返回`false`；反之`count++`

8.拼接`sql`字符串，更新票卖出去的数量

9.执行语句，如果失败，直接回滚（`Mysq_RollBack()`)并返回`false`;

10.拼接`sql`语句，插入新的预定信息

11.执行语句，失败回滚并返回`false`

12.提交事务并返回`true`

### 9.`Mysql_Begin()`；

#### 1.概要

开启事务

#### 2.具体操作

调用`mysql_query()`执行语句（“`begin`"),失败就打印出来

### 10.`Mysql_RollBack()`;

#### 1.概要

回滚事务

#### 2.具体操作

调用`mysql_query()`执行语句（“`rollback`"),失败就打印出来

### 11.`Mysql_Commit()`;

#### 1.概要

提交事务

#### 2.具体操作

调用`mysql_query()`执行语句（“`commit`"),失败就打印出来

### 12.`Db_Show_My_Ticket();`

同Db_Show_Ticket，只是查询的表不同

### 13.`Db_Qx_Ticket()`

#### 1.概要

在数据库操作得到目标票的id，删除预约记录并恢复票量

#### 2.具体操作

1.开启事务

2.拼接字符串，查询预约对应的票

3.执行语句，如果错误直接回滚

4.把结果集取出来，如果为空，回滚返回false

5.得到结果集行数,如果为0，释放结果集内存，并回滚返回false

6.得到结果集第一行的内容，获得票的id

7.释放结果集

8.拼接`sql`语句字符串，删除预约记录

9.提交语句，错误就回滚返回false

10.拼接`sql`语句字符串，恢复票数

11.执行语句，错误就回宫并返回

12.提交事务，返回真

# 客户端

## 1.连接服务器端

事实上跟服务器端的`Ser_Init()`几乎一样，只是多了一个连接而已：

1.创建自己的`socket`

2.创建`sockaddr_in`结构体对象，把服务器端的IP族类型，端口，`ip`号导入

3.与服务器端连接（connect())

### connect()

~~~ 
connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr))

1.自己的连接端点
2.服务器端的地址信息
3.结构体大小
~~~

## 2.业务

客户端的业务逻辑非常简单，主要工作分为以下几点：

1.输入需要的信息（例如手机号，密码等）

2.把需要发给服务器端的数据放进`Json`容器，发到发送缓冲区

3.接收服务器端的信息

4.解析服务器端的信息（需要`Json`容器对象和阅读器对象，`Read.parse()`)

5.将解析出来的状态内容与ok比较，一样则成功；如果有的话，把其他解析得到的数据内容放进`Json`容器

# `makefile`

自动化编译项目

# `run.sh`

自动化执行脚本