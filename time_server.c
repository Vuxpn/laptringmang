#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

void signalHandler(int signo)
{
    pid_t pid = wait(NULL);
    printf("Child process terminated, pid = %d\n", pid);
}

int main()
{
    // Tao socket cho ket noi
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Khai bao dia chi server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    // Gan socket voi cau truc dia chi
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    // Chuyen socket sang trang thai cho ket noi
    if (listen(listener, 5))
    {
        perror("listen() failed");
        return 1;
    }

    signal(SIGCHLD, signalHandler);

    while (1)
    {
        printf("Waiting for new client\n");
        int client = accept(listener, NULL, NULL);
        printf("New client accepted, client = %d\n", client);

        if (fork() == 0)
        {
            // Tien trinh con, xu ly yeu cau tu client
            // Dong socket listener
            close(listener);

            char buf[256];
            while (1)
            {
                int ret = recv(client, buf, sizeof(buf), 0);
                if (ret <= 0)
                    break;

                buf[ret] = '\0';
                printf("Received from %d: %s", getpid(), buf);

                // Kiểm tra lệnh client gửi lên
                if (strncmp(buf, "GET_TIME", 8) == 0)
                {
                    // Kiểm tra xem sau GET_TIME có dấu cách không
                    if (isspace(buf[8]))
                    {
                        // Lấy định dạng thời gian từ lệnh client
                        char format[15];
                        sscanf(buf + 8, "%s", format);

                        // Lấy thời gian hiện tại
                        time_t rawtime;
                        struct tm *timeinfo;
                        time(&rawtime);
                        timeinfo = localtime(&rawtime);

                        // Format thời gian theo định dạng yêu cầu
                        char timeString[15];
                        if (strcmp(format, "dd/mm/yyyy") == 0)
                            strftime(timeString, sizeof(timeString), "%d/%m/%Y", timeinfo);
                        else if (strcmp(format, "dd/mm/yy") == 0)
                            strftime(timeString, sizeof(timeString), "%d/%m/%y", timeinfo);
                        else if (strcmp(format, "mm/dd/yyyy") == 0)
                            strftime(timeString, sizeof(timeString), "%m/%d/%Y", timeinfo);
                        else if (strcmp(format, "mm/dd/yy") == 0)
                            strftime(timeString, sizeof(timeString), "%m/%d/%y", timeinfo);
                        else
                        {
                            // Lệnh không hợp lệ, gửi thông báo về cho client
                            char *msg = "Dinh dang khong duoc ho tro, hien chi ho tro cac dinh dang sau: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy\n";
                            send(client, msg, strlen(msg), 0);
                            continue;
                        }

                        // Gửi thời gian định dạng về client
                        sprintf(timeString, "%s\n", timeString);
                        send(client, timeString, strlen(timeString), 0);
                    }
                    else
                    {
                        // Lệnh không hợp lệ, gửi thông báo về cho client
                        char *msg = "Lenh khong hop le, vui long su dung lenh theo format GET_TIME [format]\n";
                        send(client, msg, strlen(msg), 0);
                    }
                }
                else
                {
                    // Lệnh không hợp lệ, gửi thông báo về cho client
                    char *msg = "Lenh khong hop le, vui long su dung lenh theo format GET_TIME [format]\n";
                    send(client, msg, strlen(msg), 0);
                }
            }

            // Ket thuc tien trinh con
            exit(0);
        }

        // Dong socket client o tien trinh cha
        close(client);
    }

    return 0;
}
