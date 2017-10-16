#ifndef _BOND_MODULE_H
#define _BOND_MODULE_H

#include "bond_base.h"

#include "bond_link.h"
#include "bond_diff.h"
#include "bond_parse.h"
#include "bond_trigger.h"

void bonding_module_init( struct rte_mempool *mbuf_pool );

void bonding_module_clean( void );

void bonding_do_work();

#endif
