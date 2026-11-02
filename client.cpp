#include "client.h"

bool TcpClient::Socket_Init()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        return false;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr(ips.c_str());

    int res = connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (res == -1)
    {
        cout << "connect ser err" << endl;
        return false;
    }

    return true;
}
void TcpClient::Print_Info()
{
    if (!login_status)
    {
        cout << "-------用户名：游客 -----  状态: 未登录---------" << endl;
        cout << " 1 登陆    2  注册   3 退出" << endl;
        cout << "--------------------------------------------" << endl;
        cout << "请输入要执行的操作：" << endl;
        cin >> op_type;
        if (3 == op_type)
        {
            op_type = 0;
        }
    }
    else
    {
        cout << "------用户名：" << username << "------状态：已登录-----" << endl;
        cout << "1 查看可预约的信息     2 预定" << endl;
        cout << "3 查看我的预约        4 取消预约" << endl;
        cout << "5 退出 " << endl;
        cout << "请输入要执行的操作:" << endl;
        cin >> op_type;
        if (op_type == 5)
        {
            op_type = 0;
        }
        else
        {
            op_type += 2;
        }
    }
}

void TcpClient::User_Register()
{
    cout << "请输入手机号码:" << endl;
    cin >> usertel;

    cout << "请输入用户名:" << endl;
    cin >> username;

    string passwd1, passwd2;
    cout << "请输入密码：" << endl;
    cin >> passwd1;
    cout << "请再次输入密码:" << endl;
    cin >> passwd2;

    if (passwd1.compare(passwd2) != 0)
    {
        cout << "两次输入的秘密不一致" << endl;
        return;
    }

    Json::Value val;
    val["type"] = REGISTER;
    val["user_tel"] = usertel;
    val["user_name"] = username;
    val["user_passwd"] = passwd1;

    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);

    char buff_status[128] = {0};
    int n = recv(sockfd, buff_status, 128, 0);
    Json::Value ra;
    Json::Reader Read;
    if (!Read.parse(buff_status, ra))
    {
        cout << "json无法解析:";
        cout << buff_status << endl;
        return;
    }

    string st = ra["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "注册失败" << endl;
        return;
    }

    cout << "注册成功" << endl;
    login_status = true;

    return;
}
void TcpClient::User_Login()
{
    cout << "请输入手机号码：" << endl;
    cin >> usertel;
    cout << "请输入密码:" << endl;
    string userpasswd;
    cin >> userpasswd;

    Json::Value val;
    val["type"] = LOGIN;
    val["user_tel"] = usertel;
    val["user_passwd"] = userpasswd;
    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);

    // 接收服务器信息
    char buff[128] = {0};
    int res = recv(sockfd, buff, 127, 0);
    if (res <= 0)
    {
        cout << "ser close" << endl;
        return;
    }

    Json::Value ra;
    Json::Reader Read;
    if (!Read.parse(buff, ra))
    {
        cout << "解析Json失败" << endl;
        return;
    }

    string st = ra["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "登陆失败" << endl;
        return;
    }
    username = ra["user_name"].asString();
    login_status = true;
    return;
}
void TcpClient::Show_Ticket() // 显示可预约的信息
{
    Json::Value val;
    val["type"] = SHOW;
    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);

    // 接受服务器返回的数据
    char buff[1024] = {0};
    if (recv(sockfd, buff, 1023, 0) <= 0)
    {
        cout << "ser close" << endl;
        return;
    }

    Json::Value ra;
    Json::Reader Read;
    if (!Read.parse(buff, ra))
    {
        cout << "json解析失败" << endl;
        return;
    }

    string s = ra["status"].asString();
    if (s.compare("OK") != 0)
    {
        cout << "查看预约信息失败" << endl;
        return;
    }

    int num = ra["num"].asInt();
    if (num == 0)
    {
        cout << "没有可预约的信息" << endl;
        return;
    }

    // 打印可预约的信息
    cout << "|序号|   名称   |  总票数  |  已预定  |   时间   |" << endl;
    for (int i = 0; i < num; i++)
    {
        cout << "  " << ra["ticket_arr"][i]["ticket_id"].asString();
        cout << "     " << ra["ticket_arr"][i]["ticket_name"].asString();
        cout << "     " << ra["ticket_arr"][i]["ticket_max"].asString();
        cout << "     " << ra["ticket_arr"][i]["ticket_count"].asString();
        cout << "     " << ra["ticket_arr"][i]["day_time"].asString() << endl;
    }
    cout << "--------------end------------------------------" << endl;
    cout << endl;
    return;
}
void TcpClient::Yd_Ticket() // 预定
{
    Show_Ticket();
    cout << "请输入要预定的序号:" << endl;
    int index = -1;
    cin >> index; // 用户输入的序号,检查合法性

    Json::Value val;
    val["type"] = YD;
    val["user_tel"] = usertel;
    val["ticket_id"] = to_string(index);
    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);

    char buff[128] = {0};
    int n = recv(sockfd, buff, 127, 0);
    if (n <= 0)
    {
        cout << "ser close" << endl;
        return;
    }

    Json::Value ra;
    Json::Reader Read;
    if (!Read.parse(buff, ra))
    {
        cout << "json无法解析" << endl;
        return;
    }

    string s = ra["status"].asString();
    if (s.compare("OK") != 0)
    {
        cout << "预定失败" << endl;
        return;
    }

    cout << "预定成功" << endl;
    return;
}
void TcpClient::Show_My_Ticket()
{
    Json::Value val;

    val["type"] = XSYD;
    val["user_tel"] = usertel;

    send(
        sockfd,
        val.toStyledString().c_str(),
        strlen(val.toStyledString().c_str()),
        0);

    char buff[1024] = {0};

    if (recv(sockfd, buff, 1023, 0) <= 0)
    {
        cout << "ser close" << endl;
        return;
    }

    Json::Value ra;
    Json::Reader Read;

    if (!Read.parse(buff, ra))
    {
        cout << "json解析失败" << endl;
        return;
    }

    string status = ra["status"].asString();

    if (status.compare("OK") != 0)
    {
        cout << "查看我的预约失败" << endl;
        return;
    }

    int num = ra["num"].asInt();

    if (num == 0)
    {
        cout << "您没有预约记录" << endl;
        return;
    }

    cout << "|预约编号| 手机号 | 票编号 |预约时间|" << endl;

    for (int i = 0; i < num; i++)
    {
        cout << " "
             << ra["yd_arr"][i]["yd_id"].asString();

        cout << " "
             << ra["yd_arr"][i]["tel"].asString();

        cout << " "
             << ra["yd_arr"][i]["ticket_id"].asString();

        cout << " "
             << ra["yd_arr"][i]["ctime"].asString();

        cout << endl;
    }
    cout << "--------------end--------------" << endl;
}
void TcpClient::Qx_Ticket()
{
    cout << "请输入要取消的预约编号:" << endl;

    int yd_id;
    cin >> yd_id;

    Json::Value val;

    val["type"] = QXYD;
    val["yd_id"] = to_string(yd_id);

    send(
        sockfd,
        val.toStyledString().c_str(),
        strlen(val.toStyledString().c_str()),
        0);

    char buff[128] = {0};

    int n = recv(sockfd, buff, 127, 0);

    if (n <= 0)
    {
        cout << "ser close" << endl;
        return;
    }

    Json::Value ra;

    Json::Reader Read;

    if (!Read.parse(buff, ra))
    {
        cout << "json解析失败" << endl;
        return;
    }

    string status =
        ra["status"].asString();

    if (status.compare("OK") != 0)
    {
        cout << "取消预约失败" << endl;
        return;
    }

    cout << "取消预约成功" << endl;
}
void TcpClient::Run()
{
    while (runing)
    {
        Print_Info();
        switch (op_type)
        {
        case LOGIN:
            User_Login();
            break;
        case REGISTER:
            User_Register();
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
        case SYS_EXIT:
            runing = false;
            break;

        default:
            cout << "无效输入" << endl;
            break;
        }
    }
}

int main()
{
    TcpClient cli;
    if (!cli.Socket_Init())
    {
        exit(1);
    }

    cli.Run();

    exit(0);
}