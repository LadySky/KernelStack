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

#define BONDING_TRIGGER_PIPE "/tmp/bonding_trigger_pipe"

#define OPEN_MODE O_WRONLY
#define PIPE_BUF_LEN 512

int pipe_fd ;

int trigger_init();
void trigger_clean( int pipe_fd );
void trigger_loop( int pipe_fd );

#endif
