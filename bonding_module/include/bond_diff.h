#ifndef _BOND_DIFF_H
#define _BOND_DIFF_H

#include "bond_base.h"
#include "bond_link.h"
#include "bond_parse.h"
#include "bond_actions.h"

BOND_PORT* check_diff( BOND_PORT** pphead, PORT_CACHE* port_conf );
BOND_PORT* user_add_bond( BOND_PORT** pphead, PORT_CACHE* port_conf );
BOND_PORT* user_edit_bond( BOND_PORT** pphead, BOND_PORT* diff_node, PORT_CACHE* port_conf );
int user_del_bond( BOND_PORT** pphead );

int check_mode( BOND_PORT* diff_node, PORT_CACHE* port_conf );
int check_slaves( BOND_PORT* diff_node, PORT_CACHE* port_conf );

#endif
