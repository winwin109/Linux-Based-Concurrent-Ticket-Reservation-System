#include <iostream>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

int main()
{
    vector<int> fds;

    const char *ip = "127.0.0.1";
    int port = 6000;

    for (int i = 0; i < 50000; i++)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);

        if (fd == -1)
        {
            cout << "socket创建失败，i = " << i << endl;
            break;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);

        if (connect(fd, (sockaddr *)&addr, sizeof(addr)) == -1)
        {
            cout << "connect失败，当前成功连接数 = "
                 << fds.size() << endl;

            close(fd);
            break;
        }

        fds.push_back(fd);

        if ((i + 1) % 1000 == 0)
        {
            cout << "已经成功连接："
                 << i + 1 << endl;
        }
    }

    cout << "最终成功连接数："
         << fds.size() << endl;

    // 不关闭这些socket，让连接一直保持
    cout << "保持连接，按Ctrl+C退出..." << endl;

    while (true)
    {
        sleep(10);
    }
}