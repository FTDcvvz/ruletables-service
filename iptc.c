#include <stdio.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <sys/wait.h>  
#include "linux_list.h"
  
#define SERVPORT 3333  
#define BACKLOG 10
#define BUFFER_SIZE 1024

void 
ipt_server(struct list_head * head)
{
    int    listenfd, connfd;
    struct sockaddr_in  servaddr;

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

    printf("======waiting for client's request======\n");
    while(1){
        if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }
        
        //send chain head
        int sendSize = sizeof(struct list_head);
        char * buffer = (char*)malloc(sendSize);
        memcpy(buffer,head,sendSize);

        int pos=0;
        int len=0;
        while(pos<sendSize){
            len = send(connfd,buffer+pos,BUFFER_SIZE,0);
            if(len<=0){
                perror("send error! \n");
                break;
            }
            pos+=len;
        }
        free(buffer);

        close(connfd);
    }

    close(listenfd);
}