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

	return 0;
}

void bond_trigger_clean( int pipe_fd )
{
	if ( pipe_fd < 0 ) {
		//printf( "ERROR,Invalid parameter\n" );
		return ;
	}

	memset( &bond_trigger, 0x00, sizeof(bond_trigger) );
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
	bonding_global->bond_thread_t = pth_id;
	bonding_global->thread_running = 1;
	DEBUG_INFO( "trigger thread created and detached: %lu ", pth_id );

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
	int*    pbond_running = temp->pbond_running;
	int*    pipe_fd = temp->pipe_fd;
	FILE**  bond_conf_fp = temp->pbond_conf_fp;
	char*   bond_conf_buff = temp->pbond_conf_buff;
	int*    pthread_running = temp->pthread_running;
	void**  pthread_res = temp->pthread_res;

	if ( pbond_running == NULL ) {
		DEBUG_INFO( "ERROR,Invalid pbond_runing" );
		*pthread_res = NULL;
		return NULL;
	}

	if ( bond_conf_fp == NULL ) {
		DEBUG_INFO( "ERROR,Invalid bond_conf_fp" );
		*pthread_res = NULL;
		return NULL;
	}

	if ( bond_conf_buff == NULL ) {
		DEBUG_INFO( "ERROR,Invalid bond_conf_buff" );
		*pthread_res = NULL;
		return NULL;
	}

	if ( pthread_running == NULL ) {
		DEBUG_INFO( "ERROR,Invalid pthread_running" );
		*pthread_res = NULL;
		return NULL;
	}

	if ( pthread_res == NULL ) {
		DEBUG_INFO( "ERROR,Invalid pthread_res" );
		exit( -1 );
	}

	*pipe_fd = open( BONDING_TRIGGER_PIPE, OPEN_MODE | O_NONBLOCK );
	if ( *pipe_fd < 0 ) {
		DEBUG_INFO( "ERROR,pipe open failed" );
		return NULL;
	}

    do {
        // reading from FIFO
        res = read( *pipe_fd, buffer, PIPE_BUF_LEN );
        if ( res > 0 ) {

        	// trigger bonding parse here
        	if ( strncmp( buffer, "CMD_BOND_ACT", strlen(buffer) ) == 0
        			|| strncmp( buffer, "CMD_BOND_ACT\n", strlen(buffer) ) == 0 ) {

        		// do parse here
        		FILE* fp = bond_load_conf();
        		*bond_conf_fp = fp;
        		bond_parse_conf( fp, bond_conf_buff, MAX_LINE_LEN );
        		bond_close_conf( fp );
        		fp = NULL;
        		*bond_conf_fp = NULL;

        	} else {

        		DEBUG_INFO( "Unknown command(s)" );
        		continue;
        	}
        }

        /***
         * 1s=1000ms 1ms=1000μs 1μs=1000ns
         * here sleep for 100 * 3 ms each loop
         ***/
        usleep( 1000 * 100 * 3 );

    } while( ( *pbond_running == 1 ) );

    if ( *pbond_running == 0 ) {

    	DEBUG_INFO( "thread now exit..." );
    	*pthread_res = (void*)(0x01);
    }

	*pthread_running = 0;

	return NULL;
}
