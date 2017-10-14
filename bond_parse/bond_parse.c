#include "bond_parse.h"

char* parse_init( const char* conf_path, int buff_len )
{
	char* buff = NULL;

	if ( buff_len < 1 ) {
		printf( "Invalid parameter buff_len %d\n", buff_len );
		exit(-1);
	}

	if ( conf_path == NULL ) {
		printf( "Invalid parameter conf_path\n" );
		exit(-1);
	}

	if ( access( conf_path, R_OK ) != 0 ) {
		printf( "File %s may not exists or reabable\n", conf_path );
		exit(-1);
	}

	buff = ( char* )malloc( sizeof( char ) * buff_len );
	if( buff == NULL ) {
		printf( "Failed to malloc buff\n" );
		exit(-1);
	}

	return buff;
}

void parse_clean( FILE* fp, char* conf_buff )
{
	if ( fp ) {
		close_conf( fp );
	}

	if ( conf_buff ) {
		free( conf_buff );
	}

	return ;
}

void close_conf( FILE* fp )
{
	if ( fp ) {
		fclose( fp );
	}
	return ;
}

FILE* load_conf( const char* conf_path )
{
	FILE* fp = NULL;
	if ( conf_path == NULL ) {
		printf( "Invalid parameter conf_path\n" );
		exit(-1);
	}
	fp = fopen( conf_path, "r" );
	if ( fp == NULL ) {
		printf( "Failed to load conf file\n" );
		exit(-1);
	}

	return fp;
}

int parse_conf( FILE* fp, char *conf_buff, int buff_len ) {

	if ( fp == NULL || conf_buff ==  NULL || buff_len < 1 ) {
		printf( "Invalid parameter\n" );
		return -1;
	}

	while ( ! feof( fp )) {

		memset( conf_buff, 0x00, buff_len );

		if ( fgets( conf_buff, buff_len, fp) == NULL ) {

			printf( "Error on read conf\n" );
			return -1;

		} else {

			//printf( "[%s]\n", conf_buff );
			if ( do_parse( conf_buff) != 0 ) {

				//printf( "parse error....\n" );
				continue;
			}
		}
	}

	printf( "Totally %d normal bond conf, %d err bond conf \n", bond_count, err_count );
	return 0;
}

int do_parse( char* conf_buff )
{
	if ( conf_buff == NULL ) {
		printf( "Invalid parameter\n" );
		return -1;
	}

	char bond_name[15] = { 0x00 };
	int mode = -1;
	int slaves_num = -1;
	char rest_buff[MAX_LINE_LEN] = {0x00};
	char slaves_buff[256] = { 0x00 };
	int slaves_ids[MAX_PORTS] = { -1 };
	int id_count = 0x00;
	int i = 0x00;
	//int common_count = 0x00;

	char* pos = NULL;
	char* pos_f = NULL;

	// comment
	if ( conf_buff[0] == '#' || conf_buff[0] == '\n' ) {
		//printf( "Comment\n" );
		return 0;
	}

	sscanf(conf_buff, "bond_name=%[^,]," //取到','为止
					  "mode=%d," // 取整数
					  "slaves_num=%d," //取整数
					  "%[^\n]", // 取到'\n'为止
		   bond_name, &mode, &slaves_num, rest_buff);
	//printf( "%s %d %d\n", bond_name, mode, slaves_num );

	i = strlen( bond_name );
	if ( i == 0 || i > 7 ) {
		printf( "Invalid bond_name \n");
		return -1;
	} else {
		bond_count ++ ;
	}

	if ( mode < 0 || mode > 6 ) {
		printf( "Invalid mode\n" );
		return -1;
	}

	if ( slaves_num < 1 || slaves_num > MAX_PORTS ) {
		printf( "Invalid slaves num\n" );
		return -1;
	}

	pos = rest_buff;
/*
	while ( common_count != 3 ) {

		if( *pos == ',' ) {
			common_count ++;
		}
		pos ++ ;
	}
*/
	pos_f = pos;
	while ( pos_f ) {

		if ( *pos_f == ',' || *pos_f == ';' ) {

			memset( slaves_buff, 0x00, sizeof(slaves_buff) );
			snprintf( slaves_buff, pos_f - pos + 1,  "%s", pos  );
			//printf( "%s\n", slaves_buff );
			sscanf( slaves_buff, "slave=vEth%d", &slaves_ids[ id_count ] );
			//printf( "id_count=%d, id=%d\n", id_count, slaves_ids[ id_count ] );
			pos = pos_f + 1;
			id_count++;
		}

		if ( *pos_f == ';' )
			break;

		pos_f ++;
	}

	if ( id_count != slaves_num ) {

		printf( "Error: the slave_num is %d, but %d slave(s) in conf,bond_name=\"%s\"\n",
				slaves_num, id_count, bond_name);
		if ( bond_count > 0) {
			bond_count -- ;
		}
		err_count ++ ;

		return -1;

	} else {

		printf( "device: bond=\"%s\", mode=%d, %d slave(s)=[ ", bond_name, mode, slaves_num);
		for ( i = 0x00; i < slaves_num ; i++ ) {
			printf( "%d ", slaves_ids[i] );
		}
		printf("]\n");
	}
	return 0;
}
