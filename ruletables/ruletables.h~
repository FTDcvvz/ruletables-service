#ifndef _ruletables_h
#define _ruletables_h
#include <stdint.h>
#include "linux_list.h"

typedef enum _action_type{
    TYPE_NONE,
    PREROUTING,
    INPUT,
    OUTPUT,
    FORWARD,
    POSTROUTING
}ActionType;

typedef enum _action_description{
    DESC_NONE,
    ACCEPT,
    DROP,
    QUEUE,
    RETURN
}ActionDesc;

typedef enum _proto_type{
    PROTO_NONE,
    TCP,
    UDP,
    ARP,
    ICMP
}ProtoType;


typedef enum _table_name{
        NAME_NONE,
	filter,
	nat,
	mangle
}table_name;

typedef struct basic_header
{
	uint32_t s_addr,d_addr;
	uint32_t smsk,dmsk;
	uint16_t spts[2];
	uint16_t dpts[2]; 
        ProtoType proto;
}basic_header;

typedef struct properties
{
	table_name tablename;
}properties;

typedef struct ruletable
{
	struct list_head list;

	basic_header head;
	uint8_t priority;
	ActionType actionType;
	ActionDesc actionDesc;
	int length;

	properties property;
}ruletable;
#endif
