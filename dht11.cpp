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
#include <opencv4/opencv2/opencv.hpp>
#define SCALE 1
#define FIFO_DIR "./communication.fifo"
using namespace std;
using namespace nlohmann;
int timeout;
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
const char *dest = "172.81.227.199";
#define VPATH "./video"
#define GPATH "./faces"
#define CPATH "./face_results"
#define VNAME "liveshow"
#define GNAME "image"
int vcode;
int graph_fd;
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
    sprintf(temp, "%d.%d", (databuf >> 24) & 0xff, (databuf >> 16) & 0xff);
    sprintf(humi, "%d.%d", (databuf >> 8) & 0xff, (databuf & 0xff));
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
int pid1, pid2, pid3;
void cleanUp(void)
{
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    kill(pid3, SIGKILL);
    close(connfdData);
    close(connfdGraph);
    close(connfdTick);
    close(graph_fd);
    unlink(FIFO_DIR);
}
void *graph_thread(void *args)
{
    int n;
    char file_dir[200];

    graph_fd = open(FIFO_DIR, O_RDONLY);

    while (1)
    {
        n = read(graph_fd, file_dir, 200);
        if (n == 0)
        {
            graph_fd = open(FIFO_DIR, O_RDONLY);
            continue;
        }
        file_dir[n] = 0;
        int tmp_fd = open(file_dir, O_RDONLY);
        int len = lseek(tmp_fd, 0, SEEK_END);
        lseek(tmp_fd, 0, SEEK_SET);
        int rlen = htonl(len);
        send(connfdGraph, &rlen, sizeof(rlen), 4);
        sendfile(connfdGraph, tmp_fd, 0, len);
        close(tmp_fd);
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
    exit(0);
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
int main(void)
{
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
    pthread_t test_thread, graph_thread_p;
    socketInit(&serverData, portData);
    socketInit(&serverGraph, portGraph);
    socketInit(&serverTick, portTick);
    atexit(cleanUp);
    connfdData = socket(AF_INET, SOCK_STREAM, 0);
    connfdGraph = socket(AF_INET, SOCK_STREAM, 0);
    connfdTick = socket(AF_INET, SOCK_STREAM, 0);
    if (mkfifo(FIFO_DIR, 0777) < 0)
    {
        perror("fifo failed");
        exit(1);
    }
    if (!access(GPATH, F_OK))
    {
        mkdir(GPATH, 0777);
    }
    if (!access(VPATH, F_OK))
    {
        mkdir(VPATH, 0777);
    }
    if (!access(CPATH, F_OK))
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
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("pid1 start failed");
        exit(1);
    }
    if (pid1 == 0)
    {
        char duration[20];
        char vcode_path[20];
        sprintf(duration, "%d", duration_num);
        sprintf(vcode_path, "%d", vcode);
        string tmp = (string)VPATH + "/" + VNAME;
        alarm(duration_num);
        sigsetjmp(senv1, 1);
        int pid1_c = fork();
        if (pid1_c < 0)
        {
            perror("pid1's child start up failed");
            exit(1);
        }
        if (pid1_c == 0)
        {
            execl("./video.bash", "video.bash", duration, tmp.c_str(), vcode_path, NULL);
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
            execl("./capture.bash", "capture.bash", vtmp.c_str(), duration, tmp_file.c_str(), NULL);
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
        int pid3_c;
        alarm(duration_num);
        sigsetjmp(senv1, 1);
        pid3_c = fork();
        if (pid3_c < 0)
        {
            perror("detect program start up failed");
            exit(1);
        }
        if (pid3_c == 0)
        {
            execl("./face.py", GPATH, CPATH, NULL);
        }
        while (1)
            ;
    }
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
        sendMessage(connfdData, message_box);
    }
    return 0;
}
