#include <stdio.h>
#include <string.h>
#include "linux_list.h"

LIST_HEAD(ruletables);

int main ()
{
	init_ruletables(&ruletables);
	ipt_server();
	exit(0);
}

