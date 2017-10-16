#ifndef _BOND_BASE_H
#define _BOND_BASE_H

// #define WITH_DPDK

#ifdef WITH_DPDK

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_arp.h>
#include <rte_spinlock.h>

#include <rte_devargs.h>

#include "rte_byteorder.h"
#include "rte_cpuflags.h"
#include "rte_eth_bond.h"

#endif

#include <stdint.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <inttypes.h>
#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>


#define BOND_DEBUG
#define ENABLE_PROMISCUOUS

#ifdef BOND_DEBUG
#define DEBUG_INFO(fmt,args...) \
	do {printf("File:<%s> Fun:[%s] Line:%d-->"fmt"\n", __FILE__, __FUNCTION__, __LINE__, ##args); } while( 0 )
#else
#define DEBUG_INFO(fmt,args...)
#endif

#define PRINT_MAC(addr)		printf("%02"PRIx8":%02"PRIx8":%02"PRIx8 \
		":%02"PRIx8":%02"PRIx8":%02"PRIx8,	\
		addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2], \
		addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5])

#define MAX_MODE_VALUE 7 // there are 7 bonding modes supported

#define UP_DELAY 60000

//#define BONDING_CONF "/tmp/bonding_devices.conf"
#define BONDING_CONF "/home/Bonding-2.0/bonding_module/bonding_devices.conf"
// #define BONDING_TRIGGER_PIPE "/tmp/bonding_trigger_pipe"
#define BONDING_TRIGGER_PIPE "/home/Bonding-2.0/bonding_module/bonding_trigger_pipe"

#define OPEN_MODE O_RDONLY
#define PIPE_BUF_LEN 2048
#define MAX_LINE_LEN 4096
#define MAX_SLAVE_PORTS 32 // Max slaves == 32

struct bond_port;
struct bonding_params;
struct bond_actions;
struct port_cache;
struct trigger_parameter;

int bond_pipe_fd ;
FILE *bond_conf_fp; // point to bonding_devices.conf
char *bond_conf_buff; // store each conf line
int bond_count; // how many bond port(s) configured correctly in config file
int err_count; // how many bond port(s) configured error in config file

struct bond_actions {

/* actions for bond port, callback functions: */
 int (*do_bond_create) (struct bond_port* pbond, int* slaves_ids, int slave_num, char* name, unsigned int mode);
 int (*do_bond_del) (struct bond_port* pbond);
 int (*do_bond_up) (struct bond_port* pbond);
 int (*do_bond_down) (struct bond_port* pbond);
 int (*do_slave_add) (struct bond_port* pbond, uint8_t phy_port_id);
 int (*do_slave_del) (struct bond_port* pbond, uint8_t phy_port_id);
 int (*do_mode_change) (struct bond_port* pbond, unsigned int newmode);
 int (*do_add_ip) (void);
 int (*do_del_ip) (void);

};

struct bond_port { // for each bond port

	char name[15]; // bond name
	unsigned int  mode; // bond mode,usually the valid values are 0~6
	uint8_t bond_port_id; // port id
	int slave_nums;
	uint8_t* slaves_ids; // point to an array,stores all slaves id,
						 // max length is the total physical ports number
#ifdef WITH_DPDK
	struct ether_addr addr; // bond port ether addr
#endif
	struct bond_port* next; // next node
	int marked; // 0 means not marked yet, 1 means was marked
} ;

struct bonding_params { // global variable, for bonding module in data plane

/* parameters: */
	int phy_port_num; // total physical ports number,should be fixed
	int bond_port_num; // total bonding ports number,dynamic changing
	int modes_on[MAX_MODE_VALUE]; // 0 -- mode off;
								  // 1 -- mode on;
	int bond_running;
	struct rte_mempool *mbuf_pool; // membuf pool
	struct bond_port* phead; // head pointer for a link contains all bond_ports
	struct bond_actions* actions; // operations

};

typedef struct bond_port BOND_PORT;
typedef struct bonding_params BONDING_PARAMS;

BONDING_PARAMS* bonding_global; // global variable

typedef struct port_cache {

	char port_name[15];
	int  mode;
	int  slaves_num;
	uint8_t  slaves_ids[MAX_SLAVE_PORTS];

} PORT_CACHE;

typedef struct trigger_parameter {
	int* pbond_running;
	int* pipe_fd;
} TRIGGER_PARAMETER;

#endif
