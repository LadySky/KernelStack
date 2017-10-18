#include "bond_module.h"

void bonding_module_init( struct rte_mempool *mbuf_pool )
{
	int  i = 0x00;

#ifdef WITH_DPDK
	if( mbuf_pool == NULL ){
		DEBUG_INFO( "Error,mbuf_pool == NULL" );
		exit(-1);
	}
#endif

	bonding_global = ( BONDING_PARAMS* )malloc( sizeof( BONDING_PARAMS ) );
	if( bonding_global == NULL ){

		DEBUG_INFO( "ERROR,bonding_global == NULL" );
		exit(-1);

	} else {

		memset( bonding_global, 0x00, sizeof( BONDING_PARAMS ) );
	}
	DEBUG_INFO( "created bonding_global %p", bonding_global );

#ifdef WITH_DPDK
	// set mbuf_pool
	bonding_global->mbuf_pool = mbuf_pool;
#endif

	// set link head
	bonding_global->phead = NULL;

	// no config file parsed
	bonding_global->bond_conf_fp = NULL;

	// no bond port in beginning
	bonding_global->bond_port_num = 0x00;

#if 0
	// no config file parsed
	bonding_global->bond_count = 0x00;

	// no config file parsed
	bonding_global->err_count = 0x00;
#endif

	// no pipe read now
	bonding_global->bond_pipe_fd = 0x00;

	// thread id
	bonding_global->bond_thread_t = 0x00;

	// not running now
	bonding_global->thread_running = 0x00;

	bonding_global->thread_res = NULL;

	// no config file parsed
	bonding_global->bond_conf_buff = NULL;

	// default all 7 modes(0~6) useable
	for ( i = 0x00; i < MAX_MODE_VALUE; i++ ) {
		bonding_global->modes_on[i] = 1; // 1 on; 0 off
		DEBUG_INFO( "mode %d set on", i );
	}

#ifdef WITH_DPDK
	// get all physical port numbers
	bonding_global->phy_port_num = rte_eth_dev_count();
	if ( bonding_global->phy_port_num == 0 ) {

		DEBUG_INFO("ERROR,bonding_global->phy_port_num == 0");
		/***
		 * don't forget cleaning moudle resources
		 * */
		bonding_module_clean();
		exit(-1);
	}
#else
	bonding_global->phy_port_num = 4;
#endif

	// create bond_actions
	bonding_global->actions = ( BOND_ACTIONS* )malloc( sizeof( BOND_ACTIONS ) );
	if ( bonding_global->actions == NULL ) {

		DEBUG_INFO("ERROR,bonding_global->actions == NULL");
		/***
		 * don't forget cleaning moudle resources
		 * */
		bonding_module_clean();
		exit(-1);

	} else {

#ifdef WITH_DPDK
		/* setting methods */
		bonding_global->actions->do_bond_create = bond_create;
		bonding_global->actions->do_bond_del = bond_del;

		bonding_global->actions->do_slave_add = slave_add;
		bonding_global->actions->do_slave_del = slave_del;

		bonding_global->actions->do_add_ip = add_ip;
		bonding_global->actions->do_del_ip = del_ip;

		bonding_global->actions->do_bond_up = bond_up;
		bonding_global->actions->do_bond_down = bond_down;

		bonding_global->actions->do_mode_change = mode_change;
#endif
		DEBUG_INFO( "set actions on" );

	}


	bond_trigger_init();
#if 0
	if ( bond_pipe_fd == -1 ) {
		DEBUG_INFO( "ERROR,Invalid bond_pipe_fd" );
		bonding_module_clean();
		exit(-1);
	}
#endif
	DEBUG_INFO( "bond trigger initialized" );


	bonding_global->bond_conf_buff = bond_parse_init();
	if ( bonding_global->bond_conf_buff == NULL ) {
		DEBUG_INFO( "ERROR, Invalid bond_conf_buff" );
		bonding_module_clean();
		exit(-1);
	}
	DEBUG_INFO( "bond parse initialized" );

#if 0
	bonding_global->bond_conf_fp = bond_load_conf();
	if ( bonding_global->bond_conf_fp == NULL ) {
		DEBUG_INFO( "ERROR,Invalid bond_conf_fp" );
		bonding_module_clean();
		exit(-1);
	}
	DEBUG_INFO( "conf loaded" );
#endif

}

void bonding_module_clean( void )
{
	if ( bonding_global == NULL ){
		return ;
	}

#ifdef WITH_DPDK
	int i = 0;
#endif

	BOND_PORT* pbond = NULL;
	//BOND_PORT* delnode = NULL;

	// stop bonding functions
	bonding_global->bond_running = 0x00;
	usleep( 1000 * 100 * 10 );

	if ( bonding_global->thread_running == 0x00 ) {

		if ( bonding_global->thread_res == NULL ) {

			DEBUG_INFO( "thread %lu exit with error...", bonding_global->bond_thread_t );

		} else {

			DEBUG_INFO( "thread %lu exit normally...", bonding_global->bond_thread_t );
		}
	}

	bond_parse_clean( bonding_global->bond_conf_fp, bonding_global->bond_conf_buff );
	DEBUG_INFO( "bond parse cleaned" );

	bond_trigger_clean( bonding_global->bond_pipe_fd );
	DEBUG_INFO( "bond trigger cleaned" );

	pbond = bonding_global->phead;
	// release all bond port
	while( pbond ){

#ifdef WITH_DPDK
		// set bond link down
		bonding_global->actions->do_bond_down(pbond);

		for ( i = 0; i != -1 && i < bonding_global->phy_port_num; i++ ){
			// remove slaves from this bond port
			bonding_global->actions->do_slave_del(pbond,pbond->slaves_ids[i]);
		}

		// delete bond port with dpdk api
		bonding_global->actions->do_bond_del(pbond);
#endif
		pbond = pbond->next;
	}

	// destroy link
	bond_link_destroy();
	DEBUG_INFO( "bond link cleaned" );

	if ( bonding_global->actions ){
		free( bonding_global->actions );
	}
	DEBUG_INFO( "bond actions cleaned" );

	// release this mem block
	memset( bonding_global, 0x00, sizeof( BONDING_PARAMS ) );
	free( bonding_global );
	DEBUG_INFO( "bond global cleaned" );

	// in case of wild pointer
	bonding_global = NULL;
}

void bonding_do_work() {

	if ( bonding_global->bond_running == 0 ) {

		bonding_global->bond_running = 1;

		//TRIGGER_PARAMETER* temp = &bond_trigger;
		bond_trigger.pipe_fd         =   &bonding_global->bond_pipe_fd;
		bond_trigger.pthread_res     =   &bonding_global->thread_res;
		bond_trigger.pbond_conf_fp   =   &bonding_global->bond_conf_fp;
		bond_trigger.pbond_running   =   &bonding_global->bond_running;
		bond_trigger.pbond_conf_buff =   bonding_global->bond_conf_buff;
		bond_trigger.pthread_running =   &bonding_global->thread_running;

		bond_trigger_entry( ( void* )( &bond_trigger ) );
	}

	return ;
}
