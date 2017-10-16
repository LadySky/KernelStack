#ifndef _BOND_LINK_H
#define _BOND_LINK_H

#include "bond_base.h"

int bond_link_add( BOND_PORT** pphead , BOND_PORT* pnode);

int bond_link_del( BOND_PORT** pphead , uint8_t port_id );

void bond_link_destroy( void );

BOND_PORT* bond_link_search( BOND_PORT** pphead, char* name );

#endif
