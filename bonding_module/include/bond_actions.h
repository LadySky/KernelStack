#ifndef _BOND_ACTIONS_H
#define _BOND_ACTIONS_H

#include "bond_base.h"



typedef struct bond_actions BOND_ACTIONS;

int bond_create (struct bond_port* pbond, int* slaves_ids, int slave_num, const char* name, unsigned int mode);
int bond_del (struct bond_port* pbond);

int bond_up (struct bond_port* pbond);
int bond_down (struct bond_port* pbond);

int slave_add (struct bond_port* pbond, uint8_t phy_port_id);
int slave_del (struct bond_port* pbond, uint8_t phy_port_id);

int mode_change (struct bond_port* pbond, unsigned int newmode);

int add_ip (void);
int del_ip (void);

int set_slave_port(uint8_t port_id);

int set_bond_attr(uint8_t port_id, uint16_t rxq_id, uint16_t txq_id, struct rte_mempool *mbuf_pool);


#endif
