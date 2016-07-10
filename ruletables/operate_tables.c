#include <stdio.h>
#include <string.h>
#include "ruletables.h"
#define SET_POLICY 0
#define APPEND 1
struct handle{
    int command;
    ruletable table;
}; 

extern void init_ruletables(struct list_head * ruletables);
extern void do_store(struct handle * h);

static ruletable filter_policy_in;
static ruletable filter_policy_out;
static ruletable filter_policy_for;

//static ruletable nat_policy;
//static ruletable mangle_policy;

extern void 
init_ruletables(struct list_head * ruletables)
{
	//init filter
	strcpy(filter_policy_in.property.tablename, "filter");
	strcpy(filter_policy_in.actionDesc ,"ACCEPT");
	strcpy(filter_policy_in.actionType , "INPUT");
	strcpy(filter_policy_out.property.tablename , "filter");
	strcpy(filter_policy_out.actionDesc , "ACCEPT");
	strcpy(filter_policy_out.actionType , "OUTPUT");
	strcpy(filter_policy_for.property.tablename , "filter");
	strcpy(filter_policy_for.actionDesc ,"ACCEPT");
	strcpy(filter_policy_for.actionType , "FORWARD");
	list_add_tail(&filter_policy_in.list, ruletables);
	list_add_tail(&filter_policy_out.list, ruletables);
	list_add_tail(&filter_policy_for.list, ruletables);
	//init nat
	//init mangle
}
 
extern void do_store(struct handle * h)
{
	printf("start to do store \n");
  	switch(h->command){
     	case SET_POLICY:
		break;
	case APPEND:
		break;
	default:
		break;
     }
}
