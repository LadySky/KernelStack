#include "bond_diff.h"

BOND_PORT* user_add_bond( BOND_PORT** pphead, PORT_CACHE* port_conf ) {

	if ( pphead == NULL || port_conf == NULL ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		return NULL;
	}

#ifdef WITH_DPDK
	int res = -1;
#endif

	BOND_PORT* newnode = ( BOND_PORT* )malloc( sizeof( BOND_PORT ) );
	if ( newnode == NULL ) {
		DEBUG_INFO( "ERROR,failed to alloc newnode" );
		return NULL;
	}

	memset( newnode, 0x00, sizeof( BOND_PORT ) );
	newnode->next = NULL;
	newnode->marked = 1;
	newnode->mode = port_conf->mode;
	newnode->slave_nums = port_conf->slaves_num;
	newnode->slaves_ids = ( uint8_t* )malloc( sizeof(uint8_t) * port_conf->slaves_num );
	memcpy( newnode->name, port_conf->port_name, sizeof( port_conf->port_name ) );
	memcpy( newnode->slaves_ids, port_conf->slaves_ids, sizeof( uint8_t ) * port_conf->slaves_num );

#ifdef WITH_DPDK
	res = bonding_global->actions->do_bond_create( newnode,
												   newnode->slaves_ids,
												   newnode->slave_nums,
												   newnode->name,
												   newnode->mode );
	if ( res != 0 ) { // create bond failed
		free(newnode);
		return NULL;
	} else {
		bond_link_add( pphead , newnode );
		return newnode;
	}
#else

	bond_link_add( pphead , newnode );
	DEBUG_INFO( "user add: name=%s mode=%u slave_nums=%d ",
				newnode->name,
				newnode->mode,
				newnode->slave_nums );
	int x = 0x00;
	printf( "[" );
	for ( x = 0x00; x < newnode->slave_nums; x++ ) {
			printf( "%u ", newnode->slaves_ids[x] );
	}
	printf( "]\n" );
	return newnode;

#endif

	return NULL;
}

int check_mode( BOND_PORT* diff_node, PORT_CACHE* port_conf )
{
	if ( diff_node == NULL || port_conf == NULL ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		return -1;
	}

	return ( diff_node->mode == port_conf->mode ) ? 0 : 1 ;
}

int check_slaves( BOND_PORT* diff_node, PORT_CACHE* port_conf )
{
	if ( diff_node == NULL || port_conf == NULL ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		return -1;
	}

	int i = 0x00;
	int j = 0x00;
	int count = 0x00;

	if ( diff_node->slave_nums != port_conf->slaves_num ) { // different slave numbers
		return 1;
	} else { // same slave numbers
		for( i = 0x00; i < diff_node->slave_nums; i ++ ) { // diff_node

			for ( j = 0x00; j < diff_node->slave_nums; j++ ) {

				if ( diff_node->slaves_ids[i] == port_conf->slaves_ids[j] ) {
					count ++ ;
					break;
				}
			}
		}

		if ( count != diff_node->slave_nums ) {
			return 1;
		}
	}

	return 0;
}

// the two nodes must have same name
BOND_PORT* user_edit_bond( BOND_PORT** pphead, BOND_PORT* diff_node, PORT_CACHE* port_conf )
{
	if ( pphead == NULL || diff_node == NULL || port_conf == NULL ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		return NULL;
	}

	int mode_changed = 0x00; // 0 not change; 1 changed;
	int slaves_changed = 0x00; // 0 not change; 1 changed;
	int i = 0x00;

	mode_changed = check_mode( diff_node, port_conf );
	if ( mode_changed == -1 ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		exit(-1);
	}

	slaves_changed = check_slaves( diff_node, port_conf );
	if ( slaves_changed == -1 ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		exit(-1);
	}

	if ( mode_changed == 1 || slaves_changed == 1 ) {

#ifdef WITH_DPDK
		bonding_global->actions->do_bond_down( diff_node );
#else
		DEBUG_INFO( "bond port %s down", diff_node->name );
#endif

		if ( mode_changed == 1 ) {

				DEBUG_INFO( "bond port %s mode=%d", diff_node->name, diff_node->mode );
				diff_node->mode = port_conf->mode;
#ifdef WITH_DPDK
				bonding_global->actions->do_mode_change( diff_node, diff_node->mode );
#else
				DEBUG_INFO( "change bond port %s mode to %d", diff_node->name, diff_node->mode);
#endif
		}

		if ( slaves_changed == 1 ) {

			for ( i = 0x00; i < diff_node->slave_nums; i++ ) {
#ifdef WITH_DPDK
				bonding_global->actions->do_slave_del( diff_node, diff_node->slaves_ids[i] );
#else
				DEBUG_INFO( "bond port %s remove slave %d", diff_node->name, diff_node->slaves_ids[i] );
#endif
			}
			free(diff_node->slaves_ids);
			DEBUG_INFO( "free bond port %s old slaves in link", diff_node->name );

			diff_node->slave_nums = port_conf->slaves_num;
			diff_node->slaves_ids = ( uint8_t* )malloc( sizeof(uint8_t) * port_conf->slaves_num );
			memcpy( diff_node->slaves_ids, port_conf->slaves_ids,
								sizeof(uint8_t) * port_conf->slaves_num);

			for ( i = 0x00; i < diff_node->slave_nums; i++ ) {
#ifdef WITH_DPDK
				bonding_global->actions->do_slave_add( diff_node, diff_node->slaves_ids[i] );
#else
				DEBUG_INFO( "bond port %s add slave %d", diff_node->name, diff_node->slaves_ids[i] );
#endif
			}
		}
		bonding_global->actions->do_bond_up( diff_node );
		DEBUG_INFO( "bond port %s now up", diff_node->name );
		diff_node->marked = 1;
		return diff_node;
	}

	return NULL;
}

int user_del_bond( BOND_PORT** pphead )
{
	if ( pphead == NULL ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		return -1;
	}

	//int i = 0x00;
	int count = 0x00;

	BOND_PORT* node = *pphead;
	while ( node ) {

		if( node->marked == 0 ) { // this node did not exists in conf file , so delete it

#ifdef WITH_DPDK
			bonding_global->actions->do_bond_down( node );
			bonding_global->actions->do_bond_del( node );
#else
			DEBUG_INFO( "bond port %s removed now", node->name );
#endif
			bond_link_del( pphead, node->bond_port_id );
			DEBUG_INFO( "bond port %s removed from link", node->name );
			count ++ ;
		}

		node = node->next;
	}

	node = *pphead;
	while ( node ) {

		if( node->marked == 1 ) {
			node->marked = 0; // set all back to 0(not marked), and wait for nexttime conf-file modify
			DEBUG_INFO( "bond port %s now set to unmarked", node->name );
		}
		node = node->next;
	}

	return count;
}

BOND_PORT* check_diff( BOND_PORT** pphead, PORT_CACHE* port_conf )
{
	if ( pphead == NULL || port_conf == NULL ) {
		DEBUG_INFO( "ERROR,Invalid parameter" );
		return NULL;
	}

	BOND_PORT* node = bond_link_search( pphead, port_conf->port_name );
	//int res = 0x00 ;

	if ( node == NULL ) { // add this node

		return user_add_bond( pphead, port_conf );

	} else { // modify this node

		return user_edit_bond( pphead, node, port_conf );

	}

	return NULL;
}


