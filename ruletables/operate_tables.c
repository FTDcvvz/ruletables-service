#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "ruletables.h"
#define LIST_HEAD_NUM 12

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
	INSERT ,
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

extern void init_ruletables();
extern void do_command(struct handle * h);
extern void free_all();

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

extern void 
init_ruletables()
{
	int i;
	for(i=0;i<LIST_HEAD_NUM;i++)
	{
		ruletable * filter_policy = (ruletable *)malloc(sizeof(ruletable));

		filter_policy->actionType = info[i].chainname;
		filter_policy->actionDesc = ACCEPT;
		filter_policy->property.tablename = info[i].tablename;
		filter_policy->priority = 0;
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
}

static void do_set_policy(table_name tablename ,ActionType actiontype,ActionDesc actiondesc)
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == tablename && info[i].chainname == actiontype)
			break;
	ruletable * rt;
	rt = list_entry(info[i].head->prev,ruletable,list);
	rt->actionDesc = actiondesc;

	print_info();
} 


static void 
do_append(struct handle * h)
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename && info[i].chainname == h->table.actionType)
			break;
	ruletable* t = (ruletable *)malloc(sizeof(ruletable));
	memcpy(t , &(h->table),sizeof(ruletable));
	list_add_tail(&(t->list),info[i].head->prev);
	info[i].length ++;
	print_info();
}

extern void free_all()
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
	{	
		struct list_head *pos;
		struct list_head *q;
		for (pos = info[i].head->next; pos != info[i].head;pos = q)
		{
			ruletable * rt;
			rt = list_entry(pos,ruletable,list);
			q = pos->next;
			free(rt);
			info[i].length--;
		}
		info[i].head->next = info[i].head;
		info[i].head->prev = info[i].head;
	}
}

static void do_alter(struct handle * h)
{
	
}

static void do_insert(struct handle * h)
{
	
}

static void do_delete(struct handle * h)
{
	int i;
	for(i=0;i<LIST_HEAD_NUM ;i++)
		if(info[i].tablename == h->table.property.tablename && info[i].chainname == h->table.actionType)
			break;

	if(info[i].length-1 <= h->index)	
		fprintf(stderr,"Index of deletion too big.\n");

	else 
	{
		int j;
		struct list_head * p = info[i].head;
		for(j=0;j<index;j++,p = p->next);
		ruletable * rt;
		rt = list_entry(p,ruletable,list);
		list_del(p);
		free(rt); 	
		info[i].length--;
	}
	print_info();
	
}

extern void do_command(struct handle * h)
{
	printf("start to do command \n");
  	switch(h->command){
     	case SET_POLICY:
		do_set_policy(h->table.property.tablename,h->table.actionType,h->table.actionDesc);
		printf("set policy succeed.\n");
		break;
	case APPEND :
		do_append(h);
		break;
	case INSERT :	
		do_insert(h);
		break;
	case ALTER :	
		do_alter(h);	//-R
		break;
	case DELETE :
		do_delete(h);    //-D
		break;
	case CLEAN :	
		free_all();	//-F
		init_ruletables();
		break;
	case ALLIN :
		break;
	default:
		break;
     }
}
