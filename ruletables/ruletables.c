#include <stdio.h>
#include <string.h>
#include "linux_list.h"

int main ()
{
	init_ruletables();
	ipt_server();
	exit(0);
}

