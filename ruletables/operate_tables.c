#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "ruletables.h"
#define LIST_HEAD_NUM 12
#define MAX_PRIORITY 100

LIST_HEAD(filter_in_ruletables);
LIST_HEAD(filter_out_ruletables);
LIST_HEAD(filter_forward_ruletables);

LIST_HEAD(nat_pre_ruletables);
LIST_HEAD(nat_in_ruletables);
LIST_HEAD(nat_out_ruletables);
LIST_HEAD(nat_post_ruletables);

LIST_HEAD(mangle_pre_ruletables);
LIST_HEAD(mangle_in_ruletables);
LIST_HEAD(mangle_out_ruletables);
LIST_HEAD(mangle_forward_ruletables);
LIST_HEAD(mangle_post_ruletables);

static struct ruletables_info{
	int length;
	table_name tablename;
	ActionType chainname;
	struct list_head * head;
}info[LIST_HEAD_NUM] = 
{
{0,filter,INPUT,&filter_in_ruletables},
{0,filter,OUTPUT,&filter_out_ruletables},
{0,filter,FORWARD,&filter_forward_ruletables},

{0,nat,PREROUTING,&nat_pre_ruletables},
{0,nat,INPUT,&nat_in_ruletables},
{0,nat,OUTPUT,&nat_out_ruletables},
{0,nat,POSTROUTING,&nat_post_ruletables},

{0,mangle,PREROUTING,&mangle_pre_ruletables},
{0,mangle,INPUT,&mangle_in_ruletables},
{0,mangle,OUTPUT,&mangle_out_ruletables},
{0,mangle,FORWARD,&mangle_forward_ruletables},
{0,mangle,POSTROUTING,&mangle_post_ruletables}
};

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

extern int init_ruletables();
extern int do_command(struct handle * h);
extern int free_all();

static void 
print_info()
{
	struct in_addr addr1,addr2;
  	
	ruletable * p;
	int i;
	for(i=0;i<LIST_HEAD_NUM;i++)
		list_for_each_entry(p,info[i].head,list)
		{
			memcpy(&addr1, &(p->head.s_addr), 4);
   			memcpy(&addr2, &(p->head.d_addr), 4);
			printf("ruletables info:\ntable name :%u , actionType:%u , actionDesc:%u , saddr:%s ,",p->property.tablename,p->actionType,p->actionDesc,inet_ntoa(addr1));	
			printf("daddr%s \n",inet_ntoa(addr2));
		}
}

extern int 
init_ruletables()
{
	int i;
	for(i=0;i<LIST_HEAD_NUM;i++)
	{
		ruletable * filter_policy = (ruletable *)malloc(sizeof(ruletable));

		filter_policy->actionType = info[i].chainname;
		filter_policy->actionDesc = ACCEPT;
		filter_policy->property.tablename = info[i].tablename;
		filter_policy->priority = MAX_PRIORITY;
		filter_policy->head.s_addr = 0;
		filter_policy->head.d_addr = 0;
		filter_policy->head.smsk = 0;
		filter_policy->head.dmsk = 0;
		filter_policy->head.spts[0] = 0;
		filter_policy->head.spts[1] = 0;
		filter_policy->head.dpts[0] = 0;
		filter_policy->head.dpts[1] = 0;
		filter_policy->head.proto = PROTO_NONE;
		
		list_add(&(filter_policy->list),info[i].head);
		info[i].length = 1;
	}
	print_info();
	return 1;
}

static int do_set_policy(table_name tablename ,ActionType actiontype,ActionDesc actiondesc)
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == tablename && info[i].chainname == actiontype)
			break;
	if(i == LIST_HEAD_NUM){
		alert_to_controller("表名或链名 cant find\n");
		return 0;
	}
	if(actiondesc == DESC_NONE){
		alert_to_controller("unknown policy.");
		return 0;
	}
		
	ruletable * rt;
	rt = list_entry(info[i].head->prev,ruletable,list);
	rt->actionDesc = actiondesc;
	rt->priority = MAX_PRIORITY;

	print_info();
	return 1;
} 

static int do_insert(struct handle * h)
{	
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename && info[i].chainname == h->table.actionType)
			break;
	if(i == LIST_HEAD_NUM){
		alert_to_controller("表名或链名 cant find\n");
		return 0;
	}
	
	ruletable * p;
	int j = 0;
	list_for_each_entry(p,info[i].head,list)
	{	
		j++;
		if(h->table.priority > p->priority)
			break;
	}	
	h->index = j;
	if(&p->list == info[i].head)	//priority is smallest
	{	
		ruletable* t = (ruletable *)malloc(sizeof(ruletable));
		memcpy(t , &(h->table),sizeof(ruletable));
		list_add_tail(&(t->list),info[i].head->prev);
		info[i].length ++;
		print_info();
		return 1;
	}
	else{
		//h->table insert before p
		ruletable* t = (ruletable *)malloc(sizeof(ruletable));
		memcpy(t , &(h->table),sizeof(ruletable));
		list_add_tail(&t->list,&p->list);
		info[i].length ++;
		print_info();
		return 1;
	}
}


static int 
do_append(struct handle * h)
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename && info[i].chainname == h->table.actionType)
			break;
	if(i == LIST_HEAD_NUM){
		alert_to_controller("表名或链名 cant find\n");
		return 0;
	}
	
	if(h->table.priority == 0)
	{
		if(h->index != 0)
		{
			alert_to_controller("index must be 0!\n");
			return 0;
		}
		ruletable* t = (ruletable *)malloc(sizeof(ruletable));
		memcpy(t , &(h->table),sizeof(ruletable));
		list_add_tail(&(t->list),info[i].head->prev);
		info[i].length ++;
		print_info();
		return 1;
	}
	else 
		return do_insert(h);
}

extern int free_all(struct handle * h)
{
	int i,flag = 0;
	ruletable * p;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename)
		{	
			flag = 1;
			list_for_each_entry(p,info[i].head,list)
			{
				if(info[i].length -1 == 0)
					break;

				struct list_head *pos;
				struct list_head *q;
				for (pos = info[i].head->next; pos != info[i].head && info[i].length -1 != 0;pos = q)
				{
					ruletable * rt;
					rt = list_entry(pos,ruletable,list);
					q = pos->next;
					list_del(pos);
					free(rt);
					info[i].length--;
				}
			}
		}
	if(flag == 0){
		alert_to_controller("表名 cant find\n");
		return 0;
	}
	return 1;	
}

static int do_alter(struct handle * h)
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename && info[i].chainname == h->table.actionType)
			break;
	if(i == LIST_HEAD_NUM){
		alert_to_controller("表名或链名 cant find\n");
		return 0;
	}

	if(info[i].length-1 < h->index)
	{	
		alert_to_controller("Index of deletion too big.\n");
		return 0;
	}

	int j;
	struct list_head * p = info[i].head;
	for(j=0;j<h->index;j++,p = p->next);
	ruletable * rt = list_entry(p,ruletable,list);
	if(rt){
			//priority cannot be changed!
			rt->head.s_addr = h->table.head.s_addr;
			rt->head.d_addr = h->table.head.d_addr;
			rt->head.smsk = h->table.head.smsk;
			rt->head.dmsk = h->table.head.dmsk;
			rt->head.spts[0] = h->table.head.spts[0];
			rt->head.spts[1] = h->table.head.spts[1];
			rt->head.dpts[0] = h->table.head.dpts[0];
			rt->head.dpts[1] = h->table.head.dpts[1];
			rt->head.proto = h->table.head.proto;
			rt->actionDesc = h->table.actionDesc;
	}
	else 
	{
		alert_to_controller("list_entry error,cant find ruletables.\n");	
		return 0;
	}		
	print_info();
	return 1;
}


static int do_delete(struct handle * h)
{
	int i,j;
	ruletable * p;
	int size = sizeof(ruletable);
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename && info[i].chainname == h->table.actionType)
			break;
	if(i == LIST_HEAD_NUM){
		alert_to_controller("表名或链名 cant find\n");
		return 0;
	}
	if(h->index ==0)  	
	{
		j = 0;
		list_for_each_entry(p,info[i].head,list)
		{	
			if(j<info[i].length-1 && memcmp(&(h->table) , p , size-sizeof(struct list_head) ) == 0 )
			{
				list_del(&(p->list));
				free(p);
				info[i].length --;
				break;
			}
			j++;
		}
		if(j == info[i].length)
		{
			alert_to_controller("找了半天没找到你想要删除的东西.\n");
			return 0;
		}
	}

	else if(info[i].length-1 < h->index)
	{	
		alert_to_controller("Index of deletion too big.\n");
		return 0;
	}

	else 
	{
		int j;
		struct list_head * p = info[i].head;
		for(j=0;j<h->index;j++,p = p->next);
		ruletable * rt = list_entry(p,ruletable,list);
		list_del(p);
		if(rt)
			free(rt); 	
		else 
		{
			alert_to_controller("list_entry error,cant find ruletables.\n");	
			return 0;
		}
		info[i].length--;
	}
	print_info();
	return 1;
}

static int all_in (struct handle * h)
{
	int i;
	ruletable * p;
	sleep(1);
	for(i=0;i<LIST_HEAD_NUM ;i++)
	{
		list_for_each_entry(p,info[i].head,list)
		{
			memcpy(&h->table,p,sizeof(ruletable));
			send_to_controller(h);
		}
	}
	return 0;
}

extern int do_command(struct handle * h)
{
	int ret = 0;
  	switch(h->command){
     	case SET_POLICY:
		ret = do_set_policy(h->table.property.tablename,h->table.actionType,h->table.actionDesc);
		break;
	case APPEND :
		ret = do_append(h);	//-A or  -I	
		break;
	case ALTER :	
		ret = do_alter(h);	//-R
		break;
	case DELETE :
		ret = do_delete(h);    //-D
		break;
	case CLEAN :	
		ret = free_all(h);	//-F
		break;
	case ALLIN :
		ret = all_in(h);	//send all data to controller
		break;
	default:
		alert_to_controller("输入的命令错误");
		break;
     }
	return ret;
}
