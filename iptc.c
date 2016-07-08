#include <stdio.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <sys/wait.h>  
#include<pthread.h>
#include "ruletables.h"
  
#define SERVPORT 3333  
#define SERVPORT0 6666
#define BUFFER_SIZE 10

#define SET_POLICY 0
#define APPEND 1

/*   note  :  gcc thread.c -o thread.o -lpthread */
struct handle{
    int command;
    ruletable table;
};

static void 
do_with_handle(struct handle * h,struct list_head * head);
static void
wait_for_controller(void * arg);

void 
ipt_server(struct list_head * head)
{
    int    listenfd, connfd;
    struct sockaddr_in  servaddr;
    int    listenfd0;
    struct sockaddr_in  servaddr0;
    pthread_t thread_id;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVPORT);

    if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    if( listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
/////////////////////////////////////

    if( (listenfd0 = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket0 error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    memset(&servaddr0, 0, sizeof(servaddr0));
    servaddr0.sin_family = AF_INET;
    servaddr0.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr0.sin_port = htons(SERVPORT0);

    if( bind(listenfd0, (struct sockaddr*)&servaddr0, sizeof(servaddr0)) == -1){
        printf("bind socket0 error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    if( listen(listenfd0, 10) == -1){
        printf("listen socket0 error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
//printf("%d  %d",listenfd,listenfd0);
////////////////////////////////////////
    if(pthread_create(&thread_id,NULL,(void *)(&wait_for_controller),(void *)(&listenfd0)) == -1)
    {
        fprintf(stderr,"pthread_create error!\n");
   	exit(0);
    }
//////////////////////////////////////
    printf("======waiting for client's request from iptables ======\n");
    while(1){
        if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }
        
        //recv handle
	struct handle * h = NULL;
    int recvSize = sizeof(struct handle);
    char * buffer = (char*)malloc(recvSize);
	h = (struct handle *)malloc(recvSize);

    int pos=0;
    int len=0;
    while(pos<recvSize){
        len = recv(connfd,buffer+pos,BUFFER_SIZE,0);
         if(len<=0){
            perror("send error! \n");
            break;
         }
        pos+=len;
     }   
    memcpy(h,buffer,recvSize);

    do_with_handle(h,head);

    free(buffer);
	buffer = NULL;
	free(h);
	h = NULL;
        close(connfd);
    }

    close(listenfd);
}

void
wait_for_controller(void* arg)
{
printf("======waiting for client's request from controller ======\n");
int connfd0;
int listenfd0 = *(int *)arg;
    while(1){
        if( (connfd0 = accept(listenfd0, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
           continue;
        }

        
        close(connfd0);
    }

    close(listenfd0);
}

void do_with_handle (struct handle * h,struct list_head * head)
{
    printf("info: %d %s %s %s \n",h->command,h->table.actionType,h->table.property.tablename,
        h->table.property.policy);
}
