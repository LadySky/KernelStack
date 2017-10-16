#include "bond_link.h"

int bond_link_add( BOND_PORT** pphead , BOND_PORT* pnode)
{
	if ( pnode == NULL){
		DEBUG_INFO( "ERROR,pnode == NULL" );
		return -1;
	}

	if ( *pphead == NULL ){
		*pphead = pnode;
	} else {

		BOND_PORT* p = *pphead;
		while( p->next != NULL ){
			p = p->next;
		}
		p->next = pnode;
	}

	return 0;
}

int bond_link_del( BOND_PORT** pphead , uint8_t port_id )
{
	if ( *pphead == NULL || port_id < 1 ){ // bond port id must >=2
		DEBUG_INFO( "ERROR,*pphead == NULL" );
		return -1;
	}

	BOND_PORT* pnode = *pphead;
	BOND_PORT* delnode = NULL;

	if ( pnode->bond_port_id == port_id ){ // free this head node

		*pphead = pnode->next;

		if ( pnode->slaves_ids ){
			free( pnode->slaves_ids );
		}

		free( pnode );

		return 0;
	}

	while( pnode->next != NULL ){

		if ( pnode->next->bond_port_id == port_id ){ // should be freed

			delnode = pnode->next;
			pnode->next = delnode->next;

			if ( delnode->slaves_ids ){
				free( delnode->slaves_ids );
			}

			free(delnode);

			return 0;
		} else {
			pnode = pnode->next;
		}
	}

	DEBUG_INFO( "ERROR,no port id %d found,delete failed", port_id );
	return -1;
}

void bond_link_destroy( void )
{
	BOND_PORT* delnode = NULL;
	while( bonding_global->phead ){

		delnode = bonding_global->phead;
		bonding_global->phead = delnode->next;

		if ( delnode->slaves_ids ){
			free( delnode->slaves_ids );
		}
		free(delnode);
	}
}

BOND_PORT* bond_link_search( BOND_PORT** pphead, char* name )
{
	if ( pphead == NULL || name == NULL) {
		DEBUG_INFO( "ERROR,name == NULL" );
		return NULL;
	}

	BOND_PORT* node = *pphead;
	while( node ){

		if ( strncmp( name, node->name, strlen( name ) ) == 0 ){
			return node; // matched
		} else {
			node = node->next;
		}
	}
	return NULL; // not match
}
