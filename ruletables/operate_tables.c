#include <stdio.h>
#include <string.h>
#include "ruletables.h"
typedef enum _command_list
{	SET_POLICY ,
	APPEND	,
	INSERT ,
	DELETE ,
	CLEAN  ,
	ALLIN		
}command_list;
struct handle{
    command_list command;
    ruletable table;
}; 

extern void init_ruletables(struct list_head * ruletables);
extern void do_command(struct handle * h);

static ruletable filter_policy_in;
static ruletable filter_policy_out;
static ruletable filter_policy_for;

//static ruletable nat_policy;
//static ruletable mangle_policy;

extern void 
init_ruletables(struct list_head * ruletables)
{
	//init filter
	filter_policy_in.property.tablename= filter;
	filter_policy_in.actionDesc = ACCEPT;
	filter_policy_in.actionType = INPUT;
	filter_policy_out.property.tablename =filter;
	filter_policy_out.actionDesc =ACCEPT;
	filter_policy_out.actionType =OUTPUT;
	filter_policy_for.property.tablename =filter;
	filter_policy_for.actionDesc =ACCEPT;
	filter_policy_for.actionType =FORWARD;
	list_add_tail(&filter_policy_in.list, ruletables);
	list_add_tail(&filter_policy_out.list, ruletables);
	list_add_tail(&filter_policy_for.list, ruletables);
	//init nat
	//init mangle
}
 
extern void do_command(struct handle * h)
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
