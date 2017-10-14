#ifndef _BOND_PARSE_H
#define _BOND_PARSE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BONDING_CONF "/home/cyx/MyData/workspace/bond_parse/bonding_devices.conf"

#define MAX_LINE_LEN 4096

#define MAX_PORTS 8

FILE *fp;
char *conf_buff;
int bond_count;
int err_count;

char* parse_init( const char* conf_path, int buff_len);
void parse_clean( FILE* fp, char* conf_buff );

FILE* load_conf( const char* conf_path );
int parse_conf( FILE* fp, char *conf_buff, int buff_len );
int do_parse( char* conf_buff );
void close_conf( FILE* fp );

#endif
