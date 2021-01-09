#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <pthread.h>
#include <stdlib.h>
#include </usr/include/setjmp.h>
#include <nlohmann/json.hpp>
#include <sys/fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#define DEBUG
#define _GNU_SOURCE
#define SCALE 1
#define FIFO_DIR "./communication.fifo"
//1.连接serer时会卡死(已解决：连续accept的逻辑问题)
//2.原因不明的raspivide运行失败+rtmp拉流方式，必须修改方案
//3.原因不明的ECONNRESET
//4.收到sigpipe后exec，输出不输出到终端，不明(看来不是这个问题，解决，之前重名可能是因为accept逻辑问题)
using namespace std;
using namespace nlohmann;
int timeout;
int com_port;
int comfd;
char port[10];
#define HIGH_TIME 35
int sig_num;
enum MODE
{
    CLIENT_DATA
};
int pinNumber = 29;
unsigned int databuf;
unsigned int datapool[SCALE];
const int portData = 6666;
const int portGraph = 6667;
const int portTick = 6668;
//const char *dest= "49.235.56.198";
//const char *dest= "192.168.1.105";
//const char *dest = "172.81.227.199";
const char *dest = "47.108.170.207";
#define VPATH "./video"
#define GPATH "./faces"
#define CPATH "./face_results"
#define VNAME "liveshow"
#define GNAME "image"
int vcode;
void socketInit(struct sockaddr_in *addr, int port)
{
    struct in_addr in_address;
    //printf("%p\n",addr);
    if (inet_aton(dest, &in_address) != 1)
    {
        printf("error while tansforming addr!\n");
        exit(0);
    }
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr = in_address;
};

int readSensorData(void)
{
    if (alarm(10) == -1)
    {
        perror("alarm error");
        exit(1);
    }
    unsigned int crc = 0;
    unsigned int i;
    //printf("debug:%d\n",__LINE__);
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, 0);
    delay(20);
    digitalWrite(pinNumber, 1);
    pinMode(pinNumber, INPUT); // set mode to input
    pullUpDnControl(pinNumber, PUD_UP);
    delayMicroseconds(40);
    //while(digitalRead(pinNumber));
    if (digitalRead(pinNumber) == 0) //SENSOR ANS
    {
        //printf("debug:%d\n",__LINE__);
        while (!digitalRead(pinNumber))
            ;
        //printf("debug:%d\n",__LINE__);
        while (digitalRead(pinNumber))
            ;
        //printf("debug:%d\n",__LINE__);
        for (i = 0; i < 32; i++)
        {
            //printf("debug:%d\n",__LINE__);
            while (!digitalRead(pinNumber))
                ; //data start
            //printf("debug:%d\n",__LINE__);
            delayMicroseconds(HIGH_TIME);
            databuf *= 2;
            if (digitalRead(pinNumber) == 1) //1
            {
                databuf++;
            }
            //printf("debug:%d\n",__LINE__);
            while (digitalRead(pinNumber))
                ;
            //printf("debug:%d\n",__LINE__);
        }

        for (i = 0; i < 8; i++)
        {
            //printf("debug:%d\n",__LINE__);
            while (!digitalRead(pinNumber))
                ;
            //printf("debug:%d\n",__LINE__);
            delayMicroseconds(HIGH_TIME);
            crc *= 2;
            if (digitalRead(pinNumber) == 1) //1
            {
                crc++;
            }
            //printf("debug:%d\n",__LINE__);
            while (digitalRead(pinNumber))
                ;
            //printf("debug:%d\n",__LINE__);
        }
        //printf("debug:%d\n",__LINE__);
        while (!digitalRead(pinNumber))
            ;
        //printf("debug:%d\n",__LINE__);
        pullUpDnControl(pinNumber, PUD_UP);
        // pinMode(pinNumber,OUTPUT);
        // digitalWrite(pinNumber,0);
        // delayMicroseconds(50);
        int checksum = (((databuf >> 24) & 0xff) + ((databuf >> 16) & 0xff) + ((databuf >> 8) & 0xff) + ((databuf)&0xff)) & 0xff;
#ifdef TEST
        printf("checksum test:%d %d\n", crc, checksum);
#endif
        if (crc != checksum)
        {
            printf("error:checksum error:%d %d\n", crc, checksum);
            return 0;
        }
        if (alarm(0) == -1)
        {
            perror("alarm error");
            exit(1);
        }
        return 1;
    }
    else
    {
        if (alarm(0) == -1)
        {
            perror("alarm error");
            exit(1);
        }
        return 0;
    }
}
void readData(void)
{
    int i;
    for (int i = 0; i < SCALE; i++)
    {
        // pinMode(pinNumber, OUTPUT); // set mode to output
        // digitalWrite(pinNumber, 1); // output a high level
        //delay(2000);
        databuf = 0;
        if (readSensorData() == 0)
        {
            printf("Sorry,this time of reading has failed!\n");
            //delay(1000);
            datapool[i] = 0;
            i--;
        }
        else
        {
            printf("Congratulations ! Sensor data read ok!\n");
            printf("RH:%d.%d\n", (databuf >> 24) & 0xff, (databuf >> 16) & 0xff);
            printf("TMP:%d.%d\n", (databuf >> 8) & 0xff, databuf & 0xff);
            datapool[i] = databuf;
        }
        sleep(2);
    }
}
int packer(char *message_box, unsigned int mode, const char *client_name, const char *location)
{
    int i = 0;
    int pos;
    char temp[30], humi[30];
    sprintf(humi, "%d.%d", (databuf >> 24) & 0xff, (databuf >> 16) & 0xff);
    sprintf(temp, "%d.%d", (databuf >> 8) & 0xff, (databuf & 0xff));
    json data;
    if (mode == CLIENT_DATA)
    {
        data["type"] = "data";
        data["position"] = location;
        data["name"] = client_name;
        data["temp"] = temp;
        data["humi"] = humi;
    }
    strcpy(message_box, data.dump().c_str());
    return data.dump().size();
}
void sendMessage(int connfd, char *message_box)
{
    int len = strlen(message_box);
    len = htonl(len);
    int result = write(connfd, &len, sizeof(len));
    if (result == 0 | result == -1)
    {
        printf("Data transferring error:");
        printf("%s", strerror(errno));
        exit(1);
    }
    result = write(connfd, message_box, strlen(message_box));
    if (result == 0 | result == -1)
    {
        printf("Data transferring error:");
        printf("%s", strerror(errno));
        exit(1);
    }
}
void sigPipeHandle(int signo)
{
    perror("\nsigpipe recved;the server has be shutdown:");
    sig_num = SIGPIPE;
}
static int connfdData, connfdGraph, connfdTick;
static sigjmp_buf senv1, senv2;
int jmp_code;
static int reconnect;
static struct sockaddr_in serverData, serverGraph, serverTick;
int pid1, pid2, pid3, pid4;
void notBussiness(void)
{
    printf("clarify:I'm %d\n", getpid());
    _exit(1);
}
void cleanUp(void)
{
    printf("I'm %d\n", getpid());
    close(connfdData);
    close(connfdGraph);
    close(connfdTick);
    //close(graph_fd);
    close(comfd);
    if (pid1 != 0)
        printf("debug:cleanUp:%d,pid:%d\n", __LINE__, pid1);
    kill(pid1, SIGKILL);
    if (pid2 != 0)
        printf("debug:cleanUp:%d,pid:%d\n", __LINE__, pid2);
    kill(pid2, SIGKILL);
    if (pid3 != 0)
        printf("debug:cleanUp:%d,pid:%d\n", __LINE__, pid3);
    if (pid4 != 0)
        printf("debug:cleanUp:%d,pid:%d\n", __LINE__, pid4);
    kill(pid3, SIGKILL);
    //unlink(FIFO_DIR);
#ifndef DEBUG
    int pid = fork();
    if (pid == 0)
    {
        execlp("rm", "rm", "-rf", VPATH, NULL);
    }
    pid = fork();
    if (pid == 0)
    {
        execlp("rm", "rm", "-rf", GPATH, NULL);
    }
    pid = fork();
    if (pid == 0)
    {
        execlp("rm", "rm", "-rf", CPATH, NULL);
    }
#endif
    //perror("rexec failed");
    printf("debug:before redo\n");
    int len = strlen(port);
    port[len - 1] = (((port[len - 1]) - '0') + 1) % 10 + '0';
    execl("./client", "./client", port, NULL);
    perror("rexec failed");
}
char file_dir[200];
struct sockaddr_in com_addr, com_client;
socklen_t client_len = sizeof(com_client);
void *graph_thread(void *args)
{
    int graph_fd;

    int len;
    while (1)
    {
        int n;
        graph_fd = accept(comfd, (sockaddr *)&com_client, &client_len);
        if (graph_fd < 0)
        {
            perror("accept failed");
            _exit(1);
        }
        int s_pid = fork();
        if (s_pid == 0)
        {
            atexit(notBussiness);
            n = recv(graph_fd, &len, sizeof(len), MSG_WAITALL);
            if (n < 0)
            {
                perror("read from fifo failed");
                exit(1);
            }
            if (n == 0)
            {
                close(graph_fd);
                continue;
            }
            n = recv(graph_fd, file_dir, len, MSG_WAITALL);
            if (n < 0)
            {
                perror("read from fifo failed");
                exit(1);
            }
            if (n == 0)
            {
                close(graph_fd);
                continue;
            }
            file_dir[n] = 0;
            printf("debug:gfile dir%s\n", file_dir);
            int tmp_fd = open(file_dir, O_RDONLY);
            if (tmp_fd < 0)
            {
                perror("gfd open failed");
                exit(1);
            }
            int len = lseek(tmp_fd, 0, SEEK_END);
            if (len < 0)
            {
                perror("lseek failed");
                exit(1);
            }
            printf("debug:line%d:%d\n", __LINE__, len);
            lseek(tmp_fd, 0, SEEK_SET);
            int rlen = htonl(len);
            n = send(connfdGraph, &rlen, sizeof(rlen), 4);
            if (n <= 0)
            {
                perror("send graph len to server failed");
                exit(1);
            }
            n = sendfile(connfdGraph, tmp_fd, 0, len);
            if (n <= 0)
            {
                perror("send graph to server failed");
                exit(1);
            }
            close(tmp_fd);
            close(graph_fd);
            exit(1);
        }
    }
}
void overtiming(int signo)
{
    if (reconnect == 1000)
    {
        printf("connecting overtime!\n");
        exit(0);
    }
    printf("alarm!!!timeout!!!\n");
    siglongjmp(senv1, 1);
}
void sigIntHandle(int signo)
{
    printf("recv SIGINT!\n");
    _exit(1);
}
void *testThread(void *arg)
{
    pthread_detach(pthread_self());
    int result;
    char message_box[100];
    printf("[testThread]start working!\n");
    while (result = recv(connfdData, message_box, sizeof(message_box), 0))
    {
        printf("beta:%d bytes recved\n", result);
        if (result == -1)
        {
            printf("Data transferring error:");
            printf("%s", strerror(errno));
            exit(1);
        }
    }
    if (result == 0)
    {
        printf("%s", "the server sended fin\n");
        exit(0);
    }
}
const int duration_num = 20;
const char *client_name = "yaoxuetao\'s raspi";
const char *client_pos = "yaoxuetao\'s personal office";
void *tickTock(void *args)
{
    //TO DO:ECONNRESET , use wireshark
    sigset_t smask;
    sigemptyset(&smask);
    sigaddset(&smask, SIGALRM);
    sigprocmask(SIG_SETMASK, &smask, NULL);
    string tickMessgae = (string)client_name + " still in connection\n";
    int n = 1;
    while (1)
    {
        n = send(connfdTick, tickMessgae.c_str(), tickMessgae.size(), 0);
        printf("debug:line%d:tick tock in working\n", __LINE__);
        if (n <= 0)
        {
            exit(1);
        }
        sleep(1);
    }
}
void test(void)
{
    //实验发现，fork的子进程继承exit注册
    printf("hello!exit test!\n");
}
int main(int arc, char *argv[])
{
    if (arc != 2)
    {
        printf("Enter com port!\n");
        exit(1);
    }
    strcpy(port, argv[1]);
    com_port = atoi(argv[1]);
    char message_box[1000];
    signal(SIGCHLD, SIG_DFL);
    struct sigaction sigpipe, sigint, sigalarm;
    sigemptyset(&sigpipe.sa_mask);
    sigpipe.sa_flags = 0;
    sigpipe.sa_handler = sigPipeHandle;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigint.sa_handler = sigIntHandle;
    sigemptyset(&sigalarm.sa_mask);
    sigalarm.sa_flags = 0;
    sigalarm.sa_handler = overtiming;
    sigalarm.sa_flags |= SA_RESTART;
    sigaction(SIGINT, &sigint, NULL);
    sigaction(SIGPIPE, &sigpipe, NULL);
    sigaction(SIGALRM, &sigalarm, NULL);
    pthread_t test_thread, graph_thread_p, tick_tock;
    socketInit(&serverData, portData);
    socketInit(&serverGraph, portGraph);
    socketInit(&serverTick, portTick);

    connfdData = socket(AF_INET, SOCK_STREAM, 0);
    connfdGraph = socket(AF_INET, SOCK_STREAM, 0);
    connfdTick = socket(AF_INET, SOCK_STREAM, 0);

    comfd = socket(AF_INET, SOCK_STREAM, 0);
    if (comfd < 0)
    {
        perror("comfd failed");
        exit(1);
    }
    atexit(cleanUp);
    memset(&com_addr, sizeof(com_addr), 0);
    com_addr.sin_family = AF_INET;
    com_addr.sin_port = htons(com_port);
    if (inet_aton("0.0.0.0", &com_addr.sin_addr) == 0)
    {
        perror("com addr failed");
        exit(1);
    }
    if (::bind(comfd, (sockaddr *)&com_addr, sizeof(com_addr)) < 0)
    {
        perror("com bind failed");
        exit(1);
    }
    if (listen(comfd, 10) < 0)
    {
        perror("com listen failed");
        exit(1);
    }
    //atexit(test);
    // if (mkfifo(FIFO_DIR, 0777) < 0)
    // {
    //     perror("fifo failed");
    //     exit(1);
    // }
    if (access(GPATH, F_OK))
    {
        if (mkdir(GPATH, 0777) < 0)
        {
            perror("mkdir failed");
            exit(1);
        }
    }
    if (access(VPATH, F_OK))
    {
        mkdir(VPATH, 0777);
    }
    if (access(CPATH, F_OK))
    {
        mkdir(CPATH, 0777);
    }
    //socketInit(&server_addr);
    if (connect(connfdData, (struct sockaddr *)&serverData, sizeof(serverData)) < 0)
    {
        puts(strerror(errno));
        perror("error while setting up connection to server!\n");
        exit(1);
    }
    int len, n;
    len = strlen(client_name);
    len = htonl(len);
    n = send(connfdData, &len, sizeof(len), 0);
    if (n <= 0)
    {
        perror("send name failed");
        exit(1);
    }
    n = send(connfdData, client_name, strlen(client_name), 0);
    if (n <= 0)
    {
        perror("send name failed");
        exit(1);
    }
    n = recv(connfdData, &vcode, sizeof(vcode), MSG_WAITALL);
    if (n <= 0)
    {
        perror("recv vcode failed");
        exit(1);
    }
    vcode = ntohl(vcode);
    printf("debug:vcode==%d\n", vcode);
    n = recv(connfdData, &len, sizeof(len), MSG_WAITALL);
    if (n <= 0)
    {
        perror("recv vcode failed");
        exit(1);
    }
    len = ntohl(len);
    n = recv(connfdData, message_box, len, MSG_WAITALL);
    if (n <= 0)
    {
        perror("recv vcode failed");
        exit(1);
    }
    message_box[n] = 0;
    printf("from server:%s\n", message_box);
    if (vcode < 0)
    {
        exit(1);
    }
    if (connect(connfdGraph, (struct sockaddr *)&serverGraph, sizeof(serverGraph)) < 0)
    {
        puts(strerror(errno));
        perror("error while setting up gconnection to server!\n");
        exit(1);
    }
    if (connect(connfdTick, (struct sockaddr *)&serverTick, sizeof(serverTick)) < 0)
    {
        puts(strerror(errno));
        perror("error while setting up Tconnection to server!\n");
        exit(1);
    }
    pthread_create(&test_thread, NULL, testThread, NULL);
    pthread_create(&graph_thread_p, NULL, graph_thread, NULL);
    pthread_create(&tick_tock, NULL, tickTock, NULL);
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("pid1 start failed");
        exit(1);
    }
    if (pid1 == 0)
    {
        close(comfd);
        close(connfdData);
        close(connfdGraph);
        close(connfdTick);
        if (atexit(notBussiness) < 0)
        {
            perror("register notB failed");
            exit(1);
        }
        char duration[20];
        char vcode_path[20];
        sprintf(duration, "%d", 0);
        sprintf(vcode_path, "%d", vcode);
        string tmp = (string)VPATH + "/" + VNAME;

        int id = -1;
        //更改方案
        // sigsetjmp(senv1, 1);
        // alarm(duration_num);
        int pid1_c = fork();
        id = (id + 1) % 10;
        if (pid1_c < 0)
        {
            perror("pid1's child start up failed");
            exit(1);
        }
        if (pid1_c == 0)
        {
            printf("\n\n\ndebug:video child thread%d in working\n\n\n", id);
            int fd_pid1_c = open("./pid1_c_ml", O_WRONLY | O_TRUNC | O_CREAT, 0777);
            if (fd_pid1_c < 0)
            {
                perror("video output fd open failed");
                exit(1);
            }
            dup2(fd_pid1_c, 1);
            dup2(fd_pid1_c, 2);
            //tmp += to_string(id);
            cout << "Debug: video tmpdir:" << tmp << endl;
            execl("./video.bash", "./video.bash", duration, tmp.c_str(), vcode_path, to_string(id).c_str(), NULL);
            perror("111111111111111\n1111111111111111111\n1111111111111111111");
            exit(1);
        }
        while (1)
            ;
    }
    pid2 = fork();
    if (pid2 < 0)
    {
        perror("pid1 start failed");
        exit(1);
    }
    if (pid2 == 0)
    {
        close(comfd);
        close(connfdData);
        close(connfdGraph);
        close(connfdTick);
        if (atexit(notBussiness) < 0)
        {
            perror("register notB failed");
            exit(1);
        }
        int pid2_c;
        const char *duration = "1";
        string vtmp = (string)VPATH + "/" + VNAME;
        string tmp_file = (string)GPATH + "/" + GNAME;
        sigsetjmp(senv1, 1);
        alarm(duration_num);
        pid2_c = fork();
        if (pid2_c < 0)
        {
            perror("pid2's child start up failed");
            exit(1);
        }
        if (pid2_c == 0)
        {
            int fd_pid2_c = open("./pid2_c_ml", O_WRONLY | O_TRUNC | O_CREAT, 0777);
            dup2(fd_pid2_c, 1);
            dup2(fd_pid2_c, 2);
            execl("./capture.bash", "./capture.bash", vtmp.c_str(), duration, tmp_file.c_str(), NULL);
            perror("2222222222222222\n22222222222222222\n2222222222222222");
            exit(1);
        }
        while (1)
            ;
    }
    pid3 = fork();
    if (pid3 < 0)
    {
        perror("pid1 start failed");
        exit(1);
    }
    if (pid3 == 0)
    {
        close(comfd);
        close(connfdData);
        close(connfdGraph);
        close(connfdTick);
        if (atexit(notBussiness) < 0)
        {
            perror("register notB failed");
            exit(1);
        }
        int pid3_c;
        int id = -1;
        sigsetjmp(senv1, 1);
        alarm(duration_num);
        id = (id + 1) % 200;
        pid3_c = fork();
        string tmps = (string)GPATH + "/" + GNAME + " / " + to_string(id);
        string tmpd = (string)CPATH + "/" + GNAME + "/" + to_string(id);
        if (pid3_c < 0)
        {
            perror("detect program start up failed");
            exit(1);
        }
        if (pid3_c == 0)
        {
            sleep(1);
            int fd_pid3_c = open("./pid3_c_ml", O_WRONLY | O_TRUNC | O_CREAT, 0777);
            dup2(fd_pid3_c, 1);
            dup2(fd_pid3_c, 2);
            printf("debug:%s\n", argv[1]);
            execl("./face.py", "./face.py", tmps, tmpd, argv[1], NULL);
            perror("33333333333\n33333333333333\n33333333333333");
            exit(1);
        }
        while (1)
            ;
    }
    // pid4 = fork();
    // if (pid4 == 0)
    // {
    //     close(connfdData);
    //     close(connfdGraph);
    //     close(connfdTick);
    //     if (atexit(notBussiness) < 0)
    //     {
    //         perror("register notB failed");
    //         exit(1);
    //     }
    //     int pid4_c;
    //     string tmp = (string)VPATH + "/" + VNAME;
    //     sleep(duration_num);
    //     sigsetjmp(senv1, 1);
    //     alarm(duration_num);
    //     pid4_c = fork();

    //     if (pid4_c == 0)
    //     {
    //         execlp("rm", "rm", "-f", tmp.c_str(), NULL);
    //         perror("rm proceess exec failed");
    //         exit(1);
    //     }
    // }
    if (-1 == wiringPiSetup())
    {
        printf("Setup wiringPi failed!");
        return 1;
    }
    printf("..................raspi sensor working..................\n");
    if (sigsetjmp(senv1, 1) != 0)
    {
        reconnect++;
    }
    pinMode(pinNumber, OUTPUT); // set mode to output
    digitalWrite(pinNumber, 1); // output a high level
    delay(1200);
    while (1)
    {
        readData();
        printf("data has been read!\n");
        packer(message_box, CLIENT_DATA, client_name, client_pos);
        puts(message_box);
        printf("debug:after packer\n");
        sendMessage(connfdData, message_box);
    }
    return 0;
}
