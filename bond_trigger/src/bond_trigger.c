/*
 ============================================================================
 Name        : bond_trigger.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "bond_trigger.h"

int trigger_init()
{
	int pipe_fd = -1;
	int res = -1;

	if ( access( BONDING_TRIGGER_PIPE, F_OK ) == -1 ) {
		res = mkfifo( BONDING_TRIGGER_PIPE, 0777 ); // 创建命名管道
		if ( res != 0 ) {
			printf( "ERROR,mkfifo failed\n" );
			exit(-1);
		}
	}

	pipe_fd = open( BONDING_TRIGGER_PIPE, OPEN_MODE );
	if ( pipe_fd == -1 ) {
		printf( "ERROR,pipe open failed\n" );
		exit(-1);
	}

	return pipe_fd;
}

void trigger_clean( int pipe_fd )
{
	if ( pipe_fd < 0 ) {
		printf( "ERROR,Invalid parameter\n" );
		return ;
	}

	close( pipe_fd );

	return ;
}

void trigger_loop( int pipe_fd )
{
	int res = 0x00;
	char buffer[512] = { 0x00 };

	if ( pipe_fd < 0 ) {
		printf( "ERROR,Invalid pipe_fd\n" );
		return ;
	}

    do {
        // 读取FIFO中的数据
        res = read( pipe_fd, buffer, PIPE_BUF_LEN);
        if ( res > 0 ) {

        	 // trigger bonding parse here

        }

    } while( res > 0 );

	return ;
}
