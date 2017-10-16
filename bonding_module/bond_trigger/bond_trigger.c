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

int bond_trigger_init()
{
	//int pipe_fd = -1;
	int res = -1;

	if ( access( BONDING_TRIGGER_PIPE, F_OK ) == -1 ) {
		res = mkfifo( BONDING_TRIGGER_PIPE, 0666 ); // 创建命名管道
		if ( res != 0 ) {
			DEBUG_INFO( "ERROR,mkfifo failed\n" );
			return -1;
		}
	}

	memset( &bond_trigger, 0x00, sizeof(bond_trigger) );

	//return pipe_fd;
	return 0;
}

void bond_trigger_clean( int pipe_fd )
{
	if ( pipe_fd < 0 ) {
		//printf( "ERROR,Invalid parameter\n" );
		return ;
	}

	close( pipe_fd );

	return ;
}

void* bond_trigger_entry ( void* arg )
{
	if ( arg == NULL ) {
		return NULL;
	}

	pthread_t pth_id = 0x00;
	if ( pthread_create( &pth_id, NULL, bond_trigger_loop, arg ) != 0 ) {
		DEBUG_INFO( "ERROR,thread created failed" );
		exit(-1);
	}

	pthread_detach(pth_id);
	DEBUG_INFO( "trigger thread is %lu ", pth_id );

	return NULL;
}

void* bond_trigger_loop( void* arg )
{

	if ( arg == NULL ) {
		DEBUG_INFO( "ERROR,Invalid arg\n" );
		return NULL;
	}

	int res = 0x00;
	char buffer[PIPE_BUF_LEN] = { 0x00 };

	TRIGGER_PARAMETER* temp = ( TRIGGER_PARAMETER* )arg;
	int* pbond_running = temp->pbond_running;
	int* pipe_fd = temp->pipe_fd;

	if ( pbond_running == NULL ) {
		DEBUG_INFO( "ERROR,Invalid pbond_runing" );
		return NULL;
	}

	*pipe_fd = open( BONDING_TRIGGER_PIPE, OPEN_MODE | O_NONBLOCK );
	//*pipe_fd = open( BONDING_TRIGGER_PIPE, OPEN_MODE );
	if ( *pipe_fd < 0 ) {
		DEBUG_INFO( "ERROR,pipe open failed" );
		return NULL;
	}

    do {
        // 读取FIFO中的数据
    	//DEBUG_INFO( "waitting for cmd..." );
        res = read( *pipe_fd, buffer, PIPE_BUF_LEN );
        //DEBUG_INFO( "reading cmd..." );
        if ( res > 0 ) {

        	// trigger bonding parse here
        	if ( strncmp( buffer, "CMD_BOND_ACT", strlen(buffer) - 1 ) == 0 ) {

        		DEBUG_INFO( "cmd is CMD_BOND_ACT");
        		// do parse here
        		bond_parse_conf( bond_conf_fp, bond_conf_buff, MAX_LINE_LEN );

        	} else {

        		DEBUG_INFO( "Unknown command(s)" );
        		continue;
        	}
        }

        /***
         * 1s=1000ms 1ms=1000μs 1μs=1000ns
         * here sleep for 100 ms each loop
         ***/
        usleep( 1000 * 100 * 3);

    } while( ( *pbond_running == 1 ) );

    if ( *pbond_running == 0 ) {
    	DEBUG_INFO( "thread now exit..." );
    }

	return NULL;
}
