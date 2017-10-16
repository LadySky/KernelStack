#ifndef _BOND_TRIGGER_H
#define _BOND_TRIGGER_H

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "bond_base.h"
#include "bond_parse.h"

TRIGGER_PARAMETER bond_trigger;

int bond_trigger_init();
void bond_trigger_clean( int pipe_fd );
void* bond_trigger_entry( void* arg ) ;
void* bond_trigger_loop( void* arg );

#endif
