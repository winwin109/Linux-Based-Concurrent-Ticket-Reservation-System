#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <jsoncpp/json/json.h>

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

class TcpClient
{
public:
    TcpClient(string ser_ips, short ser_port) : ips(ser_ips), port(ser_port)
    {
        runing = true;
        login_status = false;
        sockfd = -1;
        op_type = -1;
    }

    TcpClient()
    {
        ips = "127.0.0.1";
        port = 6000;
        runing = true;
        login_status = false;
        op_type = -1;
    }

    ~TcpClient()
    {
        close(sockfd);
    }

    bool Socket_Init();

    void Run();

private:
    void Print_Info();
    void User_Register();
    void User_Login();
    void Show_Ticket();//显示可预约的信息
    void Yd_Ticket();//预定
    void Show_My_Ticket();//显示我的预定
    void Qx_Ticket();
    
private:
    string ips; // 服务ip地址
    short port; // 服务器端口

    int sockfd;
    bool runing; // 客户端是否继续运行

    bool login_status; // 是否已经登陆
    int op_type;       // 用户选择的操作

    string username; // 用户名
    string usertel;  // 用户手机号码
};