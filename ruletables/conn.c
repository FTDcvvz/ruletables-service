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
#include <iptables.h>


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
#define MAX_ARG_LENGTH 50

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
static int to_argv(struct handle * h,char * argv[]);


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
    if(!send_to_kernel(h))//如果没有成功写入内核
    {
        fprintf(stderr, "Failed to write the rule to kernel\n");
    }
    else
       do_store(h);
}

static void
do_message_from_iptables(struct handle *h)
{
     printf("info from iptables: %d %s %s %s \n  next step is sending to controller \n",h->command,h->table.actionType,h->table.property.tablename,
        h->table.actionDesc);
     do_store(h);
     send_to_controller(h);
     printf("Sending to controller Succeed.\n");

}

static int 
to_argv(struct handle * h,char * argv[])
{
     int argc = 0;

     //why must strdup??!
     //char *str  is const string,If I declar buf as :char buf[] = 
    //"iptables -P FORWARD DROP".Then it does work
    //that is to say, if : char * a = "abcd";
    // I cant do this    : a[0] = '1';
    //must use strdup:返回一个指针,指向为复制字符串分配的空间;如果分配空间失败,则返回NULL值
    // int i;
    // char *bp = strdup(str);
    // printf("%s %d\n",bp,strlen(str));
    //    while((argv[i++] = strsep(&bp, " "))){  
    //            printf("%s\n", argv[i-1]);   
    //    }
    // i--;         
    // free(bp);
    // bp = NULL;
    // if(i>=MAX_ARG_LENGTH )
    // {
    //	fprintf(stderr,"too many args.\n");  
    //	exit(1);
    // }
    argv[argc++] = "iptables";
    argv[argc++] = "-t";
    argv[argc++] = h->table.property.tablename;

    switch(h->command){
     	case SET_POLICY:
		argv[argc++] = "-P";
		break;
	case APPEND:
		argv[argc++] = "-A";
		break;
	default:
		break;
     }
    argv[argc++] = h->table.actionType;

    switch(h->command){
     	case SET_POLICY:
		argv[argc++] = h->table.actionDesc;
		return argc;  //如果是set policy,到这里就结束了,return
	case APPEND:
		argv[argc++] = "-j";
		argv[argc++] = h->table.actionDesc;
		break;
	default:
		break;
     }
    
//ip地址，暂时没有考虑掩码
    struct in_addr addr1,addr2;
//static char
    static char tmp0[15],tmp1[15];
    memcpy(&addr1, &(h->table.head.s_addr), 4);
    memcpy(&addr2, &(h->table.head.d_addr), 4);
if(addr1.s_addr != 0){
    argv[argc++] = "-s";
    strcpy(tmp0 , inet_ntoa(addr1));
    argv[argc++] = tmp0;
}
if(addr2.s_addr != 0){
    argv[argc++] = "-d";
    strcpy(tmp1 , inet_ntoa(addr2));
    argv[argc++] = tmp1;
}

//   TCP OR UDP???
     unsigned int port0,port1;
     static char sp[10],dp[10];
    if((port0 = h->table.head.spts[0])!=0 || (port1 = h->table.head.spts[1])!=0)
    {
	argv[argc++] = "--sport";
	if(port0!=0 && port1!=0)
	    sprintf(sp, "%u:%u", port0,port1);
	else if(port0!=0)
            sprintf(sp, "%u", port0);
	else 
            sprintf(sp, "%u", port1);
        argv[argc++] = sp;
    }

    if((port0 = h->table.head.dpts[0])!=0 || (port1 = h->table.head.dpts[1])!=0)
    {
	argv[argc++] = "--dport";
	if(port0!=0 && port1!=0)
	    sprintf(dp, "%u:%u", port0,port1);
	else if(port0!=0)
            sprintf(dp, "%u", port0);
	else 
            sprintf(dp, "%u", port1);
        argv[argc++] = dp;
    }
int i;
for(i = 0;i<argc;i++)
	printf("%s \n",argv[i]);
   return argc;
}

static int 
send_to_kernel(struct handle * h)
{
    int argc;
    char * argv[MAX_ARG_LENGTH];
    argc = to_argv(h,argv);
    //here are the code from iptables-standalone.c
    //如果想要实现IPv6,将ip6tables-standalone.c中相应代码段copy至此,然后再用if else区分开4和6即可
     
    // if IPv4
    int ret;
    char *table = "filter";
    struct xtc_handle *handle = NULL;
    iptables_globals.program_name = "iptables";
    ret = xtables_init_all(&iptables_globals, NFPROTO_IPV4);
    if (ret < 0) {
        fprintf(stderr, "%s/%s Failed to initialize xtables\n",
                iptables_globals.program_name,
                iptables_globals.program_version);
                exit(1);
    }

#if defined(ALL_INCLUSIVE) || defined(NO_SHARED_LIBS)
    init_extensions();
    init_extensions4();
#endif

    ret = do_command4(argc, argv, &table, &handle, false);
    if (ret) {
        ret = iptc_commit(handle);
        iptc_free(handle);
    }

    if (!ret) {
        if (errno == EINVAL) {
            fprintf(stderr, "iptables: %s. "
                    "Run `dmesg' for more information.\n",
                iptc_strerror(errno));
        } else {
            fprintf(stderr, "iptables: %s.\n",
                iptc_strerror(errno));
        }
        if (errno == EAGAIN) {
            exit(RESOURCE_PROBLEM);
        }
    }
    /**  if IPv6
    int ret;
    char *table = "filter";
    struct xtc_handle *handle = NULL;

    ip6tables_globals.program_name = "ip6tables";
    ret = xtables_init_all(&ip6tables_globals, NFPROTO_IPV6);
    if (ret < 0) {
        fprintf(stderr, "%s/%s Failed to initialize xtables\n",
                ip6tables_globals.program_name,
                ip6tables_globals.program_version);
        exit(1);
    }

#if defined(ALL_INCLUSIVE) || defined(NO_SHARED_LIBS)
    init_extensions();
    init_extensions6();
#endif

    ret = do_command6(argc, argv, &table, &handle, false);
    if (ret) {
        ret = ip6tc_commit(handle);
        ip6tc_free(handle);
    }

    if (!ret) {
        if (errno == EINVAL) {
            fprintf(stderr, "ip6tables: %s. "
                    "Run `dmesg' for more information.\n",
                ip6tc_strerror(errno));
        } else {
            fprintf(stderr, "ip6tables: %s.\n",
                ip6tc_strerror(errno));
        }
    }
    **/
    return ret;
}

static void send_to_controller(struct handle * h)
{
    int    sockfd;
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


