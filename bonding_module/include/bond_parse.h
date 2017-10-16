#ifndef _BOND_PARSE_H
#define _BOND_PARSE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "bond_base.h"
#include "bond_link.h"
#include "bond_diff.h"
#include "bond_trigger.h"

char* bond_parse_init();
void bond_parse_clean( FILE* fp, char* conf_buff );

FILE* bond_load_conf();
int bond_parse_conf( FILE* fp, char *conf_buff, int buff_len );
int bond_do_parse( char* conf_buff, PORT_CACHE* port_conf );
void bond_close_conf( FILE* fp );

#endif
