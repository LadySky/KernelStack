#include "bond_base.h"
#include "bond_module.h"

#ifndef WITH_DPDK

#include <signal.h>
int test = 0x00;

void sig_handle( int sig )
{
	if ( sig == SIGINT ) {
		DEBUG_INFO( "catch signal SIGINT" );
		test = 0;
		//bonding_module_clean();
	}
	return ;
}

#endif

int main()
{

#ifndef WITH_DPDK
	test = 1;
	signal( SIGINT, sig_handle );
	bonding_module_init( NULL );
	bonding_do_work();
	while( 1 ) {
		if ( test == 0 ) {
			break;
		}
		sleep( 1 );
	}
	bonding_module_clean();
#endif
    return 0;
}
