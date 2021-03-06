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

#define MESSAGEPORT 5555
#define SERVPORT_C 6666   //server 
#define CONNPORT_C 3333 //client 
#define CONTROLLER_ADDR "127.0.0.1" 

#define BUFFER_SIZE 10
#define MAX_ARG_LENGTH 50

#define UNIX_DOMAIN "/tmp/UNIX.domain"  

/*   note  :  gcc thread.c -o thread.o -lpthread */
typedef enum _command_list
{	SET_POLICY ,
	APPEND	,
	ALTER ,
	DELETE ,
	CLEAN  ,
	ALLIN		
}command_list;
struct handle{
    int index;
    command_list command;
    ruletable table;
}; 

extern void ipt_server();
static void do_com_iptables();
static void do_com_controller();
static void do_message_from_controller(struct handle *handle);
static void do_message_from_iptables(struct handle *handle);
static int send_to_kernel(struct handle * h);
extern void send_to_controller(struct handle * h);
static int to_argv(struct handle * h,char * argv[]);


void 
ipt_server()
{
    pthread_t thread_id;
    if(pthread_create(&thread_id,NULL,(void *)(&do_com_controller),NULL) == -1)
    {
        alert_to_controller("pthread_create error!\n");
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
       int recvSize = sizeof(struct handle); 
            char * buffer = (char*)malloc(recvSize); 
        if(!buffer){ 
          alert_to_controller( "malloc error \n");
            } 
            struct handle *handle = (struct handle *)malloc(recvSize); 
            int pos=0; 
            int len=0; 
            while(pos<recvSize){ 
                len = recv(connfd,buffer+pos,BUFFER_SIZE,0); 
                if(len<=0){ 
                    perror("recv error! \n"); 
                    break; 
                } 
                pos+=len; 
            } 
            if(!memcpy(handle,buffer,recvSize)){ 
          alert_to_controller( "memcpy error : \n");
            } 
            free(buffer); 
            buffer = NULL ;
    do_message_from_controller(handle);
    
    free(handle);
    handle =NULL;
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
           int recvSize = sizeof(struct handle); 
            char * buffer = (char*)malloc(recvSize); 
        if(!buffer){ 
          alert_to_controller( "malloc error \n");
            } 
            struct handle *handle = (struct handle *)malloc(recvSize); 
            int pos=0; 
            int len=0; 
            while(pos<recvSize){ 
                len = recv(connfd,buffer+pos,BUFFER_SIZE,0); 
                if(len<=0){ 
                    perror("recv error! \n"); 
                    break; 
                } 
                pos+=len; 
            } 
            if(!memcpy(handle,buffer,recvSize)){ 
          alert_to_controller( "memcpy error \n");
            } 
            free(buffer); 
            buffer = NULL ;
    do_message_from_iptables(handle);
        free(handle);
	handle =NULL;
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
    printf("info from controller: %d %u %u %u %s ",h->command,h->table.actionType,h->table.property.tablename,h->table.actionDesc,inet_ntoa(addr1));
    printf("%s\nnext step is sending to kernel and doing the command \n",inet_ntoa(addr2));

	//这个矛盾在docommand和sendtokernel中都不好解决,因此放在外面解决掉
   if(h->table.head.proto != TCP && h->table.head.proto != UDP)
     {
	  if( h->table.head.spts[0]!=0 ||  h->table.head.spts[1]!=0 || h->table.head.dpts[0]!=0 || h->table.head.dpts[1]!=0){
		alert_to_controller("error：既不是TCP协议又不是UDP协议，但是却定义了端口号.\n");
	  }
     }

    else if(do_command(h))
 	send_to_kernel(h);
}

static void
do_message_from_iptables(struct handle *h)
{
     printf("info from iptables: %u %u %u %u \n",h->command,h->table.actionType,h->table.property.tablename,
        h->table.actionDesc);
     printf("do command...\n");
      if(do_command(h))
          send_to_controller(h);
     printf("send to controller...\n");
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
    //	alert_to_controller("too many args.\n");  
    //	exit(1);
    // }
    argv[argc++] = "iptables";
    switch(h->table.property.tablename)
    {
	case filter:
		argv[argc++] = "-t";
		argv[argc++] = "filter";
		break;
	case nat:
		argv[argc++] = "-t";
		argv[argc++] = "nat";
		break;
	case mangle:
		argv[argc++] = "-t";
		argv[argc++] = "mangle";
		break;
	default:
		break;
    }

    switch(h->command){
     	case SET_POLICY:
		argv[argc++] = "-P";
		break;
	case APPEND:		//如果优先级最低则直接append，如果优先级不是最低，那么就要插入iptables中，就要用-I	
		if(h->table.priority == 0)		
			argv[argc++] = "-A";
		else if(h->table.priority >0)
			argv[argc++] = "-I";
		break;
	case ALTER :	
		argv[argc++] = "-R";	//-R
		break;
	case DELETE :			//-D
		argv[argc++] = "-D";
		break;
	case CLEAN :	
		argv[argc++] = "-F";	//-F
		return argc;
	case ALLIN :
		break;
	default:
		break;
     }
    switch(h->table.actionType){
    	case PREROUTING:
		argv[argc++] = "PREROUTING";
		break;
	case INPUT:
		argv[argc++] = "INPUT";
		break;
    	case OUTPUT:
		argv[argc++] = "OUTPUT";
		break;
    	case FORWARD:
		argv[argc++] = "FORWARD";
		break;
    	case POSTROUTING:
		argv[argc++] = "POSTROUTING";
		break;
	default:
		break;
    }

    if(h->index>0)
    {
	static char sp[5];
	sprintf(sp, "%d", h->index);
	argv[argc++] = sp;
	if(h->command == DELETE)
		return argc;
    }

    switch(h->command){
     	case SET_POLICY:
		switch(h->table.actionDesc){
			case ACCEPT:
				argv[argc++] ="ACCEPT";
				break;
    			case DROP:
				argv[argc++] ="DROP";
				break;
    			case QUEUE:
				argv[argc++] ="QUEUE";
				break;
    			case RETURN:
				argv[argc++] ="RETURN";
				break;
			default :
				break;
		}
		break;
	case APPEND:
		argv[argc++] = "-j";
		switch(h->table.actionDesc){
			case ACCEPT:
				argv[argc++] ="ACCEPT";
				break;
    			case DROP:
				argv[argc++] ="DROP";
				break;
    			case QUEUE:
				argv[argc++] ="QUEUE";
				break;
    			case RETURN:
				argv[argc++] ="RETURN";
				break;
			default :
				break;
		}
		break;
	case DELETE:
		argv[argc++] = "-j";
		switch(h->table.actionDesc){
			case ACCEPT:
				argv[argc++] ="ACCEPT";
				break;
    			case DROP:
				argv[argc++] ="DROP";
				break;
    			case QUEUE:
				argv[argc++] ="QUEUE";
				break;
    			case RETURN:
				argv[argc++] ="RETURN";
				break;
			default :
				break;
		}
		break;
	case ALTER:
		switch(h->table.actionDesc){
			case ACCEPT:
				argv[argc++] = "-j";
				argv[argc++] ="ACCEPT";
				break;
    			case DROP:
				argv[argc++] = "-j";
				argv[argc++] ="DROP";
				break;
    			case QUEUE:
				argv[argc++] = "-j";
				argv[argc++] ="QUEUE";
				break;
    			case RETURN:
				argv[argc++] = "-j";
				argv[argc++] ="RETURN";
				break;
			default :
				break;
		}
		break;
	default:
		break;
     }
    
//ip and mask 
    struct in_addr addr1,addr2;
    struct in_addr msk1,msk2;
    static char tmp0[20],tmp1[20];
    memcpy(&addr1, &(h->table.head.s_addr), 4);
    memcpy(&addr2, &(h->table.head.d_addr), 4);
    memcpy(&msk1, &(h->table.head.smsk), 4);
    memcpy(&msk2, &(h->table.head.dmsk), 4);
if(addr1.s_addr != 0){
    argv[argc++] = "-s";
    if(!inet_ntoa(addr1)){
	alert_to_controller("source ip地址无效，请检查\n");
	return 0;
    }
    strcpy(tmp0 , inet_ntoa(addr1));
    int n = 0;
    if(msk1.s_addr != 0)
    {
	int k = 0;
   	int bits = sizeof(u_int32_t) * 8;
    	for(k = bits - 1; k >= 0; k--) {
        if (msk1.s_addr & (0x01 << k)) 
            n++;
    	}
    }
    int len = strlen(tmp0);
    if(n!=0)
    {
	tmp0[len++] = '/';
    	if(n>=10)
	{
		tmp0[len++] = '0'+n/10;
		tmp0[len++] = '0'+n%10;
	}
	else tmp0[len++] = '0'+n;
	tmp0[len] = '\0';
    }
    argv[argc++] = tmp0;
}
if(addr2.s_addr != 0){
    argv[argc++] = "-d";
    if(!inet_ntoa(addr2)){
	alert_to_controller("dest ip地址无效，请检查\n");
	return 0;
    }
    strcpy(tmp1 , inet_ntoa(addr2));
    int n = 0;
    if(msk2.s_addr != 0)
    {
	int k = 0;
   	int bits = sizeof(u_int32_t) * 8;
    	for(k = bits - 1; k >= 0; k--) {
        if (msk2.s_addr & (0x01 << k)) 
            n++;
    	}
    }
    int len = strlen(tmp1);
    if(n!=0)
    {
	tmp1[len++] = '/';
    	if(n>=10)
	{
		tmp1[len++] = '0'+n/10;
		tmp1[len++] = '0'+n%10;
	}
	else tmp1[len++] = '0'+n;
	tmp1[len] = '\0';
    }
    argv[argc++] = tmp1;
}

//   protocal
     switch(h->table.head.proto){
	case TCP:
		argv[argc++] = "-p";
		argv[argc++] = "tcp";
		break;
    	case UDP:
		argv[argc++] = "-p";
		argv[argc++] = "udp";
		break;
    	case ARP:
		argv[argc++] = "-p";
		argv[argc++] = "arp";
		break;
    	case ICMP:
		argv[argc++] = "-p";
		argv[argc++] = "icmp";
		break;
	default:
		break;
     }
     
 /*    if(h->table.head.proto != TCP && h->table.head.proto != UDP)
     {
	  if( h->table.head.spts[0]!=0 ||  h->table.head.spts[1]!=0 || h->table.head.dpts[0]!=0 || h->table.head.dpts[1]!=0){
		alert_to_controller("error：既不是TCP协议又不是UDP协议，但是却定义了端口号.\n");
		return 0;
	  }
     }
*/
//TCP UDP 
     if(h->table.head.proto == TCP || h->table.head.proto == UDP)
     {
	     unsigned int port0,port1;
	     static char sp[10],dp[10];
	  //  if((port0 = h->table.head.spts[0])!=0 || (port1 = h->table.head.spts[1])!=0)
		port0 = h->table.head.spts[0];
		port1 = h->table.head.spts[1];
	    if(port0!=0 || port1!=0)
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

	//    if((port0 = h->table.head.dpts[0])!=0 || (port1 = h->table.head.dpts[1])!=0)
		port0 = h->table.head.dpts[0];
		port1 = h->table.head.dpts[1];
	    if(port0!=0 || port1!=0)
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
     }
     
     return argc;
}

static int 
send_to_kernel(struct handle * h)
{
    int argc;
    char * argv[MAX_ARG_LENGTH] = {NULL};
    if(!(argc = to_argv(h,argv)))
	return 0;

    //here are the code from iptables-standalone.c
    //如果想要实现IPv6,将ip6tables-standalone.c中相应代码段copy至此,然后再用if else区分开4和6即可
     
    // if IPv4
    int ret;
    char *table = "filter";
    struct xtc_handle *handle = NULL;
    iptables_globals.program_name = "iptables";
    ret = xtables_init_all(&iptables_globals, NFPROTO_IPV4);
    if (ret < 0) {
        alert_to_controller( " Failed to initialize xtables\n");
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
	handle =NULL;
    }

    if (!ret) {
        if (errno == EINVAL) {
            alert_to_controller( "iptables error ,Run `dmesg' for more information.\n",
                iptc_strerror(errno));
        } else {
            alert_to_controller( "iptables error.\n");
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
    return 1;
}

extern void send_to_controller(struct handle * h)
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

extern void alert_to_controller(const char * info)
{
	int    sockfd;
    struct sockaddr_in    servaddr;
    
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MESSAGEPORT);
    if( inet_pton(AF_INET, CONTROLLER_ADDR, &servaddr.sin_addr) <= 0){
    printf("inet_pton error\n");
    exit(0);
    }

    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
    exit(0);
    }

    int needSend=strlen(info)+1;
    char *buffer = info;
    int pos=0;
    int len;
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
}


