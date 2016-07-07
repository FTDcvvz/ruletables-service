#ifndef _ruletables_h
#define _ruletables_h
#include "linux_list.h"
#include <stdint.h>

typedef struct basic_header
{
	uint32_t s_addr,d_addr;
	uint32_t smsk,dmsk;
	uint16_t spts[2];
	uint16_t dpts[2]; 
}basic_header;

typedef struct properties
{
	char tablename[10];
	
	char policy[10];
}properties;

typedef struct ruletable
{
	struct list_head list;

	basic_header head;

	int priority;
	char actionType[10];
	char actionDesc[10];

	properties property;
}ruletable;
#endif
