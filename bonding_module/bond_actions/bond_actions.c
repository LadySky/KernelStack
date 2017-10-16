#include "bond_actions.h"

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.mq_mode = ETH_MQ_RX_NONE,
		.max_rx_pkt_len = ETHER_MAX_LEN,
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload enabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_key = NULL,
			.rss_hf = ETH_RSS_IP,
		},
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

int set_slave_port(uint8_t port_id)
{

	int retval = 0x00;
	retval = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
	if ( retval != 0 ) {
		DEBUG_INFO( "ERROR,configure port fialed" );
		return -1;
	}

	retval  = rte_eth_dev_start( port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,set port start failed" );
		return -1;
	}

	return 0;
}

int set_bond_attr(uint8_t port_id, uint16_t rxq_id, uint16_t txq_id, struct rte_mempool *mbuf_pool)
{
	int retval = 0x00;

	if ( port_id < 0 || rxq_id < 0 || txq_id < 0 || mbuf_pool == NULL) {
		DEBUG_INFO( "ERROR,invalid parameters" );
		return -1;
	}

	retval = rte_eth_dev_configure( port_id, 1, 1, &port_conf );
	if (retval != 0) {
		DEBUG_INFO( "ERROR,configure port failed" );
		return -1;
	}

	/* TX setup */
	retval = rte_eth_tx_queue_setup( port_id, txq_id, RTE_TX_DESC_DEFAULT,
										rte_eth_dev_socket_id( port_id ), NULL);
	if (retval < 0) {
		DEBUG_INFO( "ERROR,tx queue setup failed" );
		return -1;
	}

	/* RX setup */
	retval = rte_eth_rx_queue_setup( port_id, rxq_id, RTE_RX_DESC_DEFAULT,
									 rte_eth_dev_socket_id( port_id ), NULL, mbuf_pool );
	if (retval < 0) {
		DEBUG_INFO( "ERROR,rx queue setup failed" );
		return -1;
	}

	return 0;
}

int bond_create (struct bond_port* pbond, int* slaves_ids, int slave_num, const char* name, unsigned int mode)
{
	int retval = 0x00;
	int i = 0x00;
	uint8_t bond_id = 0x00;

	struct ether_addr addr;

	if ( pbond == NULL || name == NULL ) {
		DEBUG_INFO( "ERROR,pbond == NULL || name == NULL" );
		return -1;
	}

	if ( slave_num > bonding_global->phy_port_num ) {
		DEBUG_INFO( "ERROR,slave_num > bonding_global->phy_port_num" );
		return -1;
	}

	if ( mode > MAX_MODE_VALUE || mode < 0x00 || bonding_global->modes_on[mode] == 0 ) {
		DEBUG_INFO( "ERROR,bonding_global->modes_on[mode] == 0" );
		return -1;
	}

	// initial physical port
	for ( i = 0; i < slave_num; i++) {
		if ( set_slave_port( slaves_ids[i] ) != 0) {
			DEBUG_INFO( "ERROR,set_slave_port" );
			return -1;
		}
	}

	// is this device already exists ?
	if ( rte_eth_dev_allocated( name ) != NULL ) {
		DEBUGU_INFO( "ERROR,bond device exists now" );
		return -1;
	}

	// create bond device
	retval = rte_eth_bond_create( name, mode, 0 /*SOCKET_ID_ANY*/);
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,Faled to create bond port" );
		return -1;
	}

	bond_id = retval;

	// set attributions for this device
	retval = set_bond_attr( bond_id, 0, 0, bonding_global->mbuf_pool );
	if ( retval != 0 ) {
		DEBUG_INFO( "ERROR,set bond attr" );
		// delete this bond device
		rte_eth_bond_free( name );
		return -1;
	}

	// add slaves
	for ( i = 0; i < slave_num; i++) {

		if ( rte_eth_bond_slave_add( bond_id, slaves_ids[i] ) < 0 ) {

			DEBUG_INFO( "ERROR,failed to add slave" );
			for ( i = 0; i < slave_num; i++) {
				rte_eth_bond_slave_remove( bond_id, slaves_ids[i] );
			}
			rte_eth_bond_free( name );
			return -1;
		}
	}

	// set UP_DELAY
	retval = rte_eth_bond_link_up_prop_delay_set(bond_id,UP_DELAY);
	if ( retval < 0 ) {

		DEBUG_INFO( "ERROR, set UP_DELAY failed" );
		for ( i = 0; i < slave_num; i++) {
			rte_eth_bond_slave_remove( bond_id, slaves_ids[i] );
		}
		rte_eth_bond_free( name );

		return -1;
	}

	// start this device
	retval  = rte_eth_dev_start( bond_id );
	if ( retval < 0 ) {

		DEBUG_INFO( "ERROR,failed to start bond device" );
		for ( i = 0; i < slave_num; i++) {
			rte_eth_bond_slave_remove( bond_id, slaves_ids[i] );
		}
		rte_eth_bond_free( name );
		return -1;
	}

#ifdef ENABLE_PROMISCUOUS
	rte_eth_promiscuous_enable( bond_id );
#endif

	// get mac addr
	memset( &addr, 0x00, sizeof(addr) );
	rte_eth_macaddr_get( bond_id, &addr);

	pbond->bond_port_id = bond_id;
	pbond->mode = mode;
	pbond->slave_nums = slave_num;
	strncpy( pbond->name, name , strlen(name) );
	memcpy( pbond->addr, &addr, sizeof(addr) );
	memcpy( pbond->slaves_ids, slaves_ids, slave_num );
	return 0;
}

int bond_del (struct bond_port* pbond)
{
	if ( pbond == NULL ) {
		DEBUG_INFO( "ERROR,invalid parameter pbond" );
		return -1
	}

	int retval = 0x00;
	int i = 0x00;

	if ( rte_eth_dev_allocated( pbond->name ) == NULL ) {
		DEBUG_INFO( "ERROR, this device not exists" );
		return -1;
	}

	retval = rte_eth_dev_stop( pbond->name );
	if ( retval != 0 ) {
		DEBUG_INFO( "ERROR,failed to stop bond device" );
		return -1;
	}

	for( i = 0x00 ; i < pbond->slave_nums; i++ ) {
		rte_eth_bond_slave_remove( pbond->bond_port_id, pbond->slaves_ids[i] );
	}

	rte_eth_bond_free( pbond->name );

	return 0;
}

int bond_up (struct bond_port* pbond)
{
	int retval = 0x00;

	if ( pbond == NULL ){
		DEBUG_INFO( "ERROR,invalid parameter pbond" );
		return -1;
	}

	if ( rte_eth_dev_allocated( pbond->name ) == NULL ) {
		DEBUG_INFO( "ERROR,bond device not exists" );
		return -1;
	}

	// start this device
	retval  = rte_eth_dev_start( bond_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to start bond device" );
		return -1;
	}

	return 0;
}

int bond_down (struct bond_port* pbond)
{
	int retval = 0x00;

	if ( pbond == NULL ) {
		DEBUG_INFO( "ERROR,invalid parameter pbond" );
		return -1;
	}

	if ( rte_eth_dev_allocated( pbond->name ) == NULL ) {
		DEBUG_INFO( "ERROR,bond device not exists" );
		return -1;
	}

	retval = rte_eth_dev_stop( pbond->bond_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to stop bond device" );
		return -1;
	}

	return 0;
}

int slave_add (struct bond_port* pbond, uint8_t phy_port_id)
{
	int retval = 0x00;
	int i = 0x00;

	if ( pbond == NULL ) {
		DEBUG_INFO( "ERROR,invalid parameter" );
		return -1;
	}

	if ( phy_port_id < 0 || phy_port_id > bonding_global->phy_port_num ) {
		DEBUG_INFO( "ERROR, invalid parameter" );
		return -1;
	}

	if ( rte_eth_dev_allocated( pbond->name ) == NULL ) {
		DEBUG_INFO( "ERROR,bond device not exists" );
		return -1;
	}

	for ( i = 0x00; i < pbond->slave_nums; i++ ) {
		if ( pbond->slaves_ids[i] == phy_port_id ) {
			return 0;
		}
	}

	if ( pbond->slave_nums + 1 > bonding_global->phy_port_num ) {
		DEBUG_INFO( "ERROR,too many slaves" );
		return -1;
	}

	retval = rte_eth_dev_stop( pbond->bond_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to stop bond device" );
		return -1;
	}

	retval = rte_eth_bond_slave_add( pbond->bond_port_id, phy_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to add slave" );
		return -1;
	}
	pbond->slaves_ids[pbond->slave_nums] = phy_port_id;
	pbond->slave_nums ++ ;

	retval = rte_eth_dev_start( pbond->bond_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to start bond device" );
		return -1;
	}

	return 0;
}

int slave_del (struct bond_port* pbond, uint8_t phy_port_id)
{
	int retval = 0x00;
	int i = 0x00;
	int exists = 0x00;
	int index = -1;
	uint8_t temp = 0x00;

	if ( pbond == NULL ) {
		DEBUG_INFO( "ERROR,invalid parameter" );
		return -1;
	}

	if ( phy_port_id < 0 || phy_port_id > bonding_global->phy_port_num ) {
		DEBUG_INFO( "ERROR, invalid parameter" );
		return -1;
	}

	if ( rte_eth_dev_allocated( pbond->name ) == NULL ) {
		DEBUG_INFO( "ERROR,bond device not exists" );
		return -1;
	}

	for ( i = 0x00; i < pbond->slave_nums; i++ ) {
		if ( pbond->slaves_ids[i] == phy_port_id ) {
			exists = 1;
			index = i;
		}
	}

	if ( exists == 0 ) {
		DEBUG_INFO( "ERROR,port id is not a slave" );
		return -1;
	}

	if ( pbond->slave_nums - 1 < 0 ) {
		DEBUG_INFO( "ERROR,no slave exists" );
		return -1;
	}

	retval = rte_eth_dev_stop( pbond->bond_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to stop bond device" );
		return -1;
	}

	retval = rte_eth_bond_slave_remove( pbond->bond_port_id, phy_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to add slave" );
		return -1;
	}
	temp = pbond->slaves_ids[index];
	pbond->slaves_ids[index] = pbond->slaves_ids[pbond->slave_nums];
	pbond->slaves_ids[pbond->slave_nums] = temp;
	pbond->slave_nums -- ;

	retval = rte_eth_dev_start( pbond->bond_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to start bond device" );
		return -1;
	}

	return 0;
}

int mode_change (struct bond_port* pbond, unsigned int newmode)
{
	int retval;

	if ( pbond == NULL ) {
		DEBUG_INFO( "ERROR,invalid parameter pbond" );
		return -1;
	}

	if ( newmode < 0 || newmode > 6 ) {
		DEBUG_INFO("ERROR,invalid parameter newmode");
		return -1;
	}

	if ( newmode == pbond->mode ) {
		return 0;
	}

	if ( bonding_global->modes_on[newmode] == 0 ) {
		DEBUG_INFO( "ERROR,this mode is disabled" );
		return -1;
	}

	retval = rte_eth_dev_stop( pbond->bond_port_id );
	if ( retval != 0 ) {
		DEBUG_INFO( "ERROR,failed to stop bond device" );
		return -1;
	}

	retval = rte_eth_bond_mode_set( pbond->bond_port_id, newmode );
	if ( retval != 0 ) {
		DEBUG_INFO( "ERROR,set mode failed" );
		return -1;
	}

	retval = rte_eth_dev_start( pbond->bond_port_id );
	if ( retval < 0 ) {
		DEBUG_INFO( "ERROR,failed to start bond device" );
		return -1;
	}

	pbond->mode = newmode;
	return 0;
}

int add_ip ( void )
{

	return 0;
}

int del_ip ( void )
{

	return 0;
}
