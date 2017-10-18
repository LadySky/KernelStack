#include "bond_parse.h"

char* bond_parse_init()
{

	char* buff = NULL;
	const char* conf_path = BONDING_CONF;
	int buff_len = MAX_LINE_LEN;

	if ( buff_len < 1 ) {
		DEBUG_INFO( "Invalid parameter buff_len %d", buff_len );
		exit(-1);
	}

	if ( conf_path == NULL ) {
		DEBUG_INFO( "Invalid parameter conf_path" );
		exit(-1);
	}

	if ( access( conf_path, R_OK ) != 0 ) {
		DEBUG_INFO( "File %s may not exists or reabable", conf_path );
		exit(-1);
	}

	buff = ( char* )malloc( sizeof( char ) * buff_len );
	if( buff == NULL ) {
		DEBUG_INFO( "Failed to malloc buff" );
		exit(-1);
	}

	return buff;
}

void bond_parse_clean( FILE* fp, char* conf_buff )
{
	if ( fp ) {
		bond_close_conf( fp );
	}

	if ( conf_buff ) {
		free( conf_buff );
	}

	return ;
}

void bond_close_conf( FILE* fp )
{
	if ( fp ) {
		fclose( fp );
	}
	return ;
}

FILE* bond_load_conf()
{
	FILE* fp = NULL;
	const char* conf_path = BONDING_CONF;

	if ( conf_path == NULL ) {
		DEBUG_INFO( "Invalid parameter conf_path" );
		exit(-1);
	}

	fp = fopen( conf_path, "r" );
	if ( fp == NULL ) {
		DEBUG_INFO( "Failed to load conf file" );
		exit(-1);
	}

	return fp;
}

int bond_parse_conf( FILE* fp, char *conf_buff, int buff_len ) {

	if ( fp == NULL || conf_buff ==  NULL || buff_len < 1 ) {
		printf( "Invalid parameter\n" );
		return -1;
	}

	PORT_CACHE port_conf;
	int res = 0x00;
	FILE* pfp = fp;

	memset( conf_buff, 0x00, buff_len );
	memset( &port_conf, 0x00, sizeof(port_conf) );

	while ( fgets( conf_buff, buff_len, pfp ) ) {

		if ( conf_buff[0] == '#' || conf_buff[0] == '\n') {

			continue;
		}

		if ( bond_do_parse( conf_buff, &port_conf ) < 0 ) { // a line configured error, ignore...

			continue;

		} else { // find differences, and synchronous differences from conf-file to bond-port

			if ( check_diff( &bonding_global->phead, &port_conf ) == NULL ) {

				DEBUG_INFO( "ERROR,failed on check_diff" );
					exit(-1);
			}
		}

		memset( &port_conf, 0x00, sizeof(port_conf) );
	}

	res = user_del_bond( &bonding_global->phead );
	if ( res == -1 ) {

		DEBUG_INFO( "ERROR,Invalid parameter" );
		exit(-1);

	} else {

		DEBUG_INFO( "%d bond port(s) deleted ", res);
	}

	DEBUG_INFO( "totally we have %d bond port(s)", bonding_global->bond_port_num );
	SHOW_BONDS( bonding_global->phead, struct bond_port*);
	return 0;
}

int bond_do_parse( char* conf_buff, PORT_CACHE* port_conf )
{
	if ( conf_buff == NULL || port_conf == NULL ) {
		printf( "Invalid parameter\n" );
		return -1;
	}

	char bond_name[15] = { 0x00 };
	unsigned int mode = 0x00;
	int slaves_num = -1;
	char rest_buff[MAX_LINE_LEN] = {0x00};
	char slaves_buff[256] = { 0x00 };
	uint8_t slaves_ids[MAX_SLAVE_PORTS] = { -1 };
	int id_count = 0x00;
	int i = 0x00;

	char* pos = NULL;
	char* pos_f = NULL;

	// comment
	if ( conf_buff[0] == '#' || conf_buff[0] == '\n' ) {
		return -1;
	}

	sscanf(conf_buff, "bond_name=%[^,]," //取到','为止
					  "mode=%u," // 取整数
					  "slaves_num=%d," // 取整数
					  "%[^\n]", // 取到'\n'为止
		   bond_name, &mode, &slaves_num, rest_buff);

#if 0
	DEBUG_INFO( "%s %d %d", bond_name, mode, slaves_num );
#endif

	i = strlen( bond_name );
	if ( i == 0 || i > 7 ) {

		DEBUG_INFO( "Invalid bond_name");
		return -1;

	}

	if ( mode < 0 || mode > 6 || bonding_global->modes_on[mode] != 1 ) {
		DEBUG_INFO( "Invalid mode" );
		return -1;
	}

	if ( slaves_num < 1 || slaves_num > MAX_SLAVE_PORTS ) {
		DEBUG_INFO( "Invalid slaves num" );
		return -1;
	}

	pos = rest_buff;

#if 0
	while ( common_count != 3 ) {

		if( *pos == ',' ) {
			common_count ++;
		}
		pos ++ ;
	}
#endif

	pos_f = pos;
	while ( pos_f ) {

		if ( *pos_f == ',' || *pos_f == ';' ) {

			memset( slaves_buff, 0x00, sizeof(slaves_buff) );
			snprintf( slaves_buff, pos_f - pos + 1,  "%s", pos  );
#if 0
			DEBUG_INFO( "%s\n", slaves_buff );
#endif
			/*
			 *  char                  --          %c或%hhd    %c采用字符身份，%hhd采用数字身份；
			 *  unsigned char         --          %c或%hhu
             *  short                 --          %hd
             *  unsigned short        --          %hu
             *  long                  --          %ld
             *  unsigned long         --          %lu
             *  int                   --          %d
             *  unsigned int          --          %u
             *  float                 --          %f或%g      %f会保留小数点后面无效的0，%g则不会；
             *  double                --          %lf或%lg
			 */
			sscanf( slaves_buff, "slave=vEth%hhu", &slaves_ids[ id_count ] );
#if 0
			DEBUG_INFO( "id_count=%d, id=%d\n", id_count, slaves_ids[ id_count ] );
#endif
			pos = pos_f + 1;
			id_count++;
		}

		if ( *pos_f == ';' )
			break;

		pos_f ++;
	}

	if ( id_count != slaves_num ) { // configured error

		DEBUG_INFO( "Error: the slave_num is %d, but %d slave(s) in conf file,bond_name=\"%s\"",
				slaves_num, id_count, bond_name);

		return -1;

	} else { // configured ok

		// summary the content of this line
		port_conf->mode = mode;
		port_conf->slaves_num = slaves_num;
		memcpy( port_conf->port_name, bond_name, sizeof(bond_name) );
		memcpy( port_conf->slaves_ids, slaves_ids, sizeof(slaves_ids) );
	}
	return 0;
}
