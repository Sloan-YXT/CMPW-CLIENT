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
#define SCALE 3
typedef unsigned char uint8;
typedef unsigned int  uint16;
int timeout;
#define HIGH_TIME 50
int reopen=0;
int sig_num;
enum MODE
{
    CLIENT_DATA
};
int pinNumber = 29;  
unsigned int databuf;
unsigned int datapool[SCALE];
const int port = 6666;
//const char *dest= "49.235.56.198";
//const char *dest= "192.168.1.105";
const char *dest = "172.81.227.199";
void  socketInit(struct sockaddr_in *addr)
{
    struct  in_addr in_address;
    //printf("%p\n",addr);
    if(inet_aton(dest,&in_address)!=1)
{
    printf("error while tansforming addr!\n");
    exit(0);
}
    memset(addr,0,sizeof(*addr));
    addr->sin_family=AF_INET;
    addr->sin_port=htons(port);
    addr->sin_addr = in_address;
}
;

uint8 readSensorData(void)
{
    if(alarm(10)==-1)
    {
        perror("alarm error");
        exit(1);
    }
    unsigned int crc=0; 
    unsigned int i;
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, 0); 
    delay(20);
    pinMode(pinNumber, INPUT); // set mode to input
    pullUpDnControl(pinNumber, PUD_UP);
    delayMicroseconds(40);
    //while(digitalRead(pinNumber));
    if (digitalRead(pinNumber) == 0) //SENSOR ANS
    {
        while (!digitalRead(pinNumber));
        while(digitalRead(pinNumber));
        for (i = 0; i < 32; i++)
        {
            while (!digitalRead(pinNumber))
                ; //data start
            delayMicroseconds(HIGH_TIME);
            databuf *= 2;
            if (digitalRead(pinNumber) == 1) //1
            {
                databuf++;
            }
            while(digitalRead(pinNumber));
        }

        for (i = 0; i < 8; i++)
        {
            while (!digitalRead(pinNumber)); 
            delayMicroseconds(HIGH_TIME);
            crc *= 2;  
            if (digitalRead(pinNumber) == 1) //1
            {
                crc++;
            }
            while(digitalRead(pinNumber));
        }
        pinMode(pinNumber,OUTPUT);
        digitalWrite(pinNumber,0);
        delayMicroseconds(50);
        pullUpDnControl(pinNumber, PUD_UP);
        //digitalWrite(pinNumber,1);
         
        int checksum = (((databuf>>24)&0xff)+((databuf>>16)&0xff)+((databuf>>8)&0xff)+((databuf)&0xff))&0xff;
        #ifdef TEST
        printf("checksum error:%d %d\n",crc,checksum);
        #endif
        if(crc!=checksum)
        {
            printf("error:checksum error:%d %d\n",crc,checksum);
            return 0;
        }
        if(alarm(0)==-1)
        {
        perror("alarm error");
        exit(1);
        }
        return 1;
    }
    else
    {
        if(alarm(0)==-1)
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
    for(int i=0;i<SCALE;i++)
    {
        // pinMode(pinNumber, OUTPUT); // set mode to output
        // digitalWrite(pinNumber, 1); // output a high level 
        databuf = 0;
        if(readSensorData()==0)
        {
            printf("Sorry,this time of reading has failed!\n");
            //delay(1000);
            datapool[i]=0;
            i--;
            continue;
        }
        else
        {
            printf("Congratulations ! Sensor data read ok!\n");
            printf("RH:%d.%d\n", (databuf >> 24) & 0xff, (databuf >> 16) & 0xff); 
            printf("TMP:%d.%d\n", (databuf >> 8) & 0xff, databuf & 0xff);
        }
        datapool[i]=databuf;
        delay(2000);
    }
}
int packer(char*message_box,unsigned int mode,const char *client_name)
{
    int i=0;
    int pos;
    unsigned int temp,humi;
    if(mode==CLIENT_DATA)
    {   
        pos = 0;
        pos+=sprintf(message_box,"{\"sensor\":[");
        while(i<SCALE)
        {
        humi=datapool[i]>>16;
        temp=datapool[i]&0xffff;
        pos+=sprintf(message_box+pos,"{\"clientName\":\"%16s\",\"temp\":\"%16x\",\"humi\":\"%16x\"}",client_name,temp,humi);
        //pos+=strlen("\"sensor\":{\"clientName\":\"\",\"temp\":\"\",\"humi\":\"\"}")+48; client_name is longer than 16 by just 1
        if(i!=SCALE-1)
        {
            pos+=sprintf(message_box+pos,",");
        }
        i++;
        }
        pos+=sprintf(message_box+pos,"]}");
        puts(message_box);
        return pos;
    }
}
void sendMessage(int connfd,char *message_box)
{
    //int result=send(connfd,message_box,strlen(message_box),0);
    //timeout:reconnect:or result in sync error
    /*
    if(alarm(100)==-1)
    {
        perror("alarm error");
        exit(1);
    }
    */
    int len = strlen(message_box);
    len = htonl(len);
    int result = write(connfd,&len,sizeof(len));
    if(result==0|result==-1)
    {
        printf("Data transferring error:");
        printf("%s",strerror(errno));
        reopen = 1;
    }
    result = write(connfd,message_box,strlen(message_box));
    if(result==0|result==-1)
    {
        printf("Data transferring error:");
        printf("%s",strerror(errno));
        reopen = 1;
    }
    /*
   if(alarm(0)==-1)
    {
        perror("alarm error");
        exit(1);
    }*/
}
void sigPipeHandle(int signo)
{
    perror("\nsigpipe recved;the server has be shutdown:");
    sig_num = SIGPIPE;
}
static int connfd;
static sigjmp_buf senv;
static int reconnect;
static struct sockaddr_in server_addr;
void overtiming(int signo)
{
    if(reconnect==1000)
    {
        printf("connecting overtime!\n");
        exit(0);
    }
    close(connfd);
    connfd = socket(AF_INET,SOCK_STREAM,0);
    printf("alarm!!!timeout!!!\n");
    sleep(10);
    if(connect(connfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {          
    puts(strerror(errno));
    perror("error while reconnecting to server!\n");
    raise(SIGINT);
    }
    siglongjmp(senv,1);
}
void sigIntHandle(int signo)
{
    close(connfd);
    printf("recv SIGINT!\n");
    exit(0);
}
void* testThread(void *arg)
{
    pthread_detach(pthread_self());
    int result;
    char message_box[100];
    printf("[testThread]start working!\n");
    while(result=recv(connfd,message_box,sizeof(message_box),0))
    {
        printf("beta:%d bytes recved\n",result);
        if(result==-1)
        {
            printf("Data transferring error:");
            printf("%s",strerror(errno));
            reopen=1;
        }
    }
    if(result==0)
    {
        printf("%s","the server sended fin\n");
        exit(0);
    }
}
int main(void)
{  
    
    struct sigaction sigpipe,sigint,sigalarm;
    sigemptyset(&sigpipe.sa_mask);
    sigpipe.sa_flags=0;
    sigpipe.sa_handler = sigPipeHandle;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags=0;
    sigint.sa_handler = sigIntHandle;
    sigemptyset(&sigalarm.sa_mask);
    sigalarm.sa_flags=0;
    sigalarm.sa_handler = overtiming;
    sigalarm.sa_flags|=SA_RESTART;
    sigaction(SIGINT,&sigint,NULL);
    sigaction(SIGPIPE,&sigpipe,NULL);
    sigaction(SIGALRM,&sigalarm,NULL);
    pthread_t test_thread;
    pthread_attr_t test_attr;
    pthread_attr_init(&test_attr);
    int policy;
    pthread_attr_getschedpolicy(&test_attr,&policy);
    printf("BETA INTIAL SCHEDULE:");
    switch(policy)
    {
        case SCHED_OTHER:printf("SCHED_OTHER\n");break;
        case SCHED_FIFO:printf("SCHED_FIFO\n");break;
        case SCHED_RR:printf("SCHED_RR");break;
    }
    socketInit(&server_addr);
    connfd = socket(AF_INET,SOCK_STREAM,0);
    //socketInit(&server_addr);
    printf("123456\n");
    if(connect(connfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        puts(strerror(errno));
        perror("error while setting up connection to server!\n");
        exit(0);
    }
    printf("78910\n");
    pthread_create(&test_thread,NULL,testThread,NULL);
    struct sched_param test_priority;
    test_priority.sched_priority = sched_get_priority_max(SCHED_OTHER);
    sched_setscheduler(test_thread,SCHED_OTHER,&test_priority);
    const char *client_name="yaoxuetao\'s raspi";
    char message_box[1000]={0};
    if (-1 == wiringPiSetup()) {
        printf("Setup wiringPi failed!");
        return 1;
    }
    // pinMode(pinNumber, OUTPUT); // set mode to output
    // digitalWrite(pinNumber, 1); // output a high level
    // delay(2000);
    printf("..................raspi sensor working..................\n");
    if(sigsetjmp(senv,1)!=0)
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
        packer(message_box,CLIENT_DATA,client_name);
        sendMessage(connfd,message_box);
        if(sig_num==SIGPIPE)
        {
            close(connfd);
            printf("%s\n","exiting..........");
            exit(0);
        }
        if(reopen==1)
        {
            close(connfd);
            connfd = socket(AF_INET,SOCK_STREAM,0);
            sleep(10);
            if(connect(connfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
        {          
        puts(strerror(errno));
        perror("error while reconnecting to server!\n");
        exit(0);
        }
            reopen = 0;
        }
        delay(1000);
    }
    return 0;
}
