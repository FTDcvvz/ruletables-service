#include <stdio.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <sys/socket.h> 
#include <sys/un.h>  
#include <sys/wait.h>  
#include <pthread.h>
#include <arpa/inet.h>
#include "ruletables.h"

#define GET_MES(handle_type)  \
            int recvSize = sizeof(struct handle_type); \
            char * buffer = (char*)malloc(recvSize); \
        if(!buffer){ \
          fprintf(stderr, "malloc error : %s\n",strerror(errno));\
            } \
            struct handle_type *handle = (struct handle_type *)malloc(recvSize); \
            int pos=0; \
            int len=0; \
            while(pos<recvSize){ \
                len = recv(connfd,buffer+pos,BUFFER_SIZE,0); \
                if(len<=0){ \
                    perror("recv error! \n"); \
                    break; \
                } \
                pos+=len; \
            } \
            if(!memcpy(handle,buffer,recvSize)){ \
          fprintf(stderr, "memcpy error : %s\n",strerror(errno));\
            } \
            free(buffer); \
            buffer = NULL 

#define SERVPORT_C 6666   //server 
#define CONNPORT_C 3333 //client 
#define CONTROLLER_ADDR "127.0.0.1" 

#define BUFFER_SIZE 10

#define SET_POLICY 0
#define APPEND 1

#define UNIX_DOMAIN "/tmp/UNIX.domain"  

/*   note  :  gcc thread.c -o thread.o -lpthread */
struct handle{
    int command;
    ruletable table;
};

extern void ipt_server();
static void do_com_iptables();
static void do_com_controller();
static void do_message_from_controller(struct handle *handle);
static void do_message_from_iptables(struct handle *handle);
static int send_to_kernel(struct handle * h);
static void send_to_controller(struct handle * h);
static char * to_iptables_string(struct handle * h);


void 
ipt_server()
{
    pthread_t thread_id;
    if(pthread_create(&thread_id,NULL,(void *)(&do_com_controller),NULL) == -1)
    {
        fprintf(stderr,"pthread_create error!\n");
        exit(0);
    }
    do_com_iptables();
}

static void
do_com_controller(){
    int    listenfd,connfd;
    struct sockaddr_in  servaddr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVPORT_C);
    if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    if( listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    printf("======waiting for controller's request ======\n");
    while(1){
        if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }
    
    printf("messages coming from controller.\n");
    GET_MES(handle);
    do_message_from_controller(handle);
    
    free(handle);
    close(connfd);
    }
    close(listenfd);
}

static void 
do_com_iptables(){
    int    listenfd,connfd;
    int len;
    struct sockaddr_un servaddr,clt_addr;

    if((listenfd=socket(PF_UNIX,SOCK_STREAM,0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sun_family=AF_UNIX;
    strncpy(servaddr.sun_path,UNIX_DOMAIN,sizeof(servaddr.sun_path)-1);  
    unlink(UNIX_DOMAIN); 

    if( bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    if( listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    printf("======waiting for iptables's request ======\n");
    while(1){
    len=sizeof(clt_addr); 
        if((connfd = accept(listenfd, (struct sockaddr*)&clt_addr,&len)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }
        GET_MES(handle);
    do_message_from_iptables(handle);

        free(handle);
        close(connfd);
    }
    close(listenfd);
}

static void
do_message_from_controller(struct handle *h)
{
     struct in_addr addr1,addr2;
     memcpy(&addr1, &(h->table.head.s_addr), 4);
     memcpy(&addr2, &(h->table.head.d_addr), 4);
     printf("info from controller: %d %s %s %s %s ",h->command,h->table.actionType,h->table.property.tablename,
        h->table.actionDesc,inet_ntoa(addr1));
     printf("%s\n  next step is sending to kernel and store it \n",inet_ntoa(addr2));
     if(send_to_kernel(h))//如果写入内核成功
        do_store(h);
}

static void
do_message_from_iptables(struct handle *h)
{
     printf("info from iptables: %d %s %s %s \n  next step is sending to controller \n",h->command,h->table.actionType,h->table.property.tablename,
        h->table.actionDesc);
     send_to_controller(h);
     printf("Sending to controller Succeed.\n");

}

static char * 
to_iptables_string(struct handle * h)
{
    char * ret_string;
    ret_string = "test";
    return ret_string;
}

static 
int send_to_kernel(struct handle * h)
{
    char * i_string ;
    i_string = to_iptables_string(h);
    printf("%s \n",i_string);
    return 1;
}

static void send_to_controller(struct handle * h)
{
    int    sockfd, n;
    struct sockaddr_in    servaddr;
    
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(CONNPORT_C);
    if( inet_pton(AF_INET, CONTROLLER_ADDR, &servaddr.sin_addr) <= 0){
    printf("inet_pton error\n");
    exit(0);
    }

    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
    exit(0);
    }

    int needSend=sizeof(struct handle); 

    char *buffer=(char*)malloc(needSend);
  
    int pos=0;
    int len;
    memcpy(buffer,h,needSend);
    while(pos < needSend){
            len = send(sockfd, buffer+pos, BUFFER_SIZE, 0);
            if (len < 0)
            {
                printf("Send Data Failed!\n");
                break;
            }
            pos+=len;
    }
    close(sockfd);
    free(buffer);
    buffer = NULL;
}


