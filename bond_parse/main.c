#include "bond_parse.h"

int main()
{
	conf_buff = parse_init ( BONDING_CONF, MAX_LINE_LEN);
	fp = load_conf( BONDING_CONF );
	parse_conf( fp, conf_buff, MAX_LINE_LEN );
	parse_clean( fp, conf_buff );
}
