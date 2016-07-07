#include <stdio.h>
#include <string.h>
#include "ruletables.h"

LIST_HEAD(ruletables);
ruletable filter_policy_in;
ruletable filter_policy_out;
ruletable filter_policy_for;

ruletable nat_policy;
ruletable mangle_policy;

void 
init_ruletables()
{
	//init filter
	strcpy(filter_policy_in.property.tablename, "filter");
	strcpy(filter_policy_in.property.policy ,"ACCEPT");
	strcpy(filter_policy_in.actionType , "INPUT");
	strcpy(filter_policy_out.property.tablename , "filter");
	strcpy(filter_policy_out.property.policy , "ACCEPT");
	strcpy(filter_policy_out.actionType , "OUTPUT");
	strcpy(filter_policy_for.property.tablename , "filter");
	strcpy(filter_policy_for.property.policy ,"ACCEPT");
	strcpy(filter_policy_for.actionType , "FORWARD");
	list_add_tail(&filter_policy_in.list, &ruletables);
	list_add_tail(&filter_policy_out.list, &ruletables);
	list_add_tail(&filter_policy_for.list, &ruletables);
	//init nat
	//init mangle
}


int main ()
{
	//init the chain
	init_ruletables();
	//
	//iptables，port 3333
	ipt_server(&ruletables);
	//controller，port 6666

	exit(0);
}
