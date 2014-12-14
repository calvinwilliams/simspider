#include "HashX.h"

/* ¶àÎ¬¹þÏ£Ëã·¨ */

static BOOL FreeDuplicate( void *pv )
{
	free( pv );

	return TRUE;
}

static int FreeMDHashTreeLeaf( struct MDHashTree *pmdht , struct MDHashNode **pp_node )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( (*pp_node)->value )
	{
		if( (*pp_node)->pfuncFreeMDHashNodeProc )
			(*pp_node)->pfuncFreeMDHashNodeProc( (*pp_node)->value );
		(*pp_node)->value = NULL ;
		
		pmdht->key_count--;
		pmdht->total_value_size -= (*pp_node)->value_len ;
	}
	
	return 0;
}

static int AllocMDHashNode( struct MDHashTree *pmdht , struct MDHashNode **pp_node , char *key , void *value , long value_len , BOOL (*pfuncFreeMDHashNodeProc)(void *pv) , int mode )
{
	char		*tmp = NULL ;
	
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( (*pp_node) == NULL )
	{
		(*pp_node) = malloc( sizeof(struct MDHashNode) ) ;
		if( (*pp_node) == NULL )
			return HASH_RETCODE_ERROR_ALLOC;
		memset( (*pp_node) , 0x00 , sizeof(struct MDHashNode) );
	}
	
	if( (*key) )
	{
		return AllocMDHashNode( pmdht , (struct MDHashNode **)( (*pp_node)->a_key + (*key) ) , key + 1 , value , value_len , pfuncFreeMDHashNodeProc , mode );
	}
	else
	{
		if( (*pp_node)->value )
		{
			if( ( mode & HASH_PUTMODE_ADD ) )
				return HASH_RETCODE_ERROR_KEY_EXIST;
		}
		else
		{
			if( ( mode & HASH_PUTMODE_REPLACE ) )
				return HASH_RETCODE_ERROR_KEY_NOT_EXIST;
		}
		
		if( ( mode & HASH_PUTMODE_DUPLICATE ) )
		{
			tmp = (char*)malloc( value_len + 1 ) ;
			if( tmp == NULL )
				return HASH_RETCODE_ERROR_ALLOC;
			memcpy( tmp , value , value_len );
			tmp[ value_len ] = '\0' ;
			
			value = (void*)tmp ;
		}
		
		if( (*pp_node)->value )
		{
			if( (*pp_node)->pfuncFreeMDHashNodeProc )
				(*pp_node)->pfuncFreeMDHashNodeProc( (*pp_node)->value );
			
			pmdht->total_value_size -= (*pp_node)->value_len ;
			pmdht->key_count--;
		}
		
		(*pp_node)->value = value ;
		(*pp_node)->value_len = value_len ;
		if( ( mode & HASH_PUTMODE_DUPLICATE ) )
			(*pp_node)->pfuncFreeMDHashNodeProc = & FreeDuplicate ;
		else
			(*pp_node)->pfuncFreeMDHashNodeProc = pfuncFreeMDHashNodeProc ;
		
		pmdht->total_value_size += value_len ;
		pmdht->key_count++;
		
		return 0;
	}
}

static int QueryMDHashNode( struct MDHashNode *p_node , char *key , struct MDHashNode **pp_node , void **pp_value , long *p_value_len )
{
	void		**pp = NULL ;
	
	if( p_node == NULL )
	{
		return HASH_RETCODE_ERROR_KEY_NOT_EXIST;
	}
	
	if( (*key) )
	{
		pp = p_node->a_key + (*key) ;
		if( (*pp) )
		{
			return QueryMDHashNode( (*pp) , key + 1 , pp_node , pp_value , p_value_len );
		}
		else
		{
			return HASH_RETCODE_ERROR_KEY_NOT_EXIST;
		}
	}
	else
	{
		if( p_node->value == NULL )
		{
			return HASH_RETCODE_ERROR_KEY_NOT_EXIST;
		}
		
		if( pp_node )
			(*pp_node) = p_node ;
		if( pp_value )
			(*pp_value) = p_node->value ;
		if( p_value_len )
			(*p_value_len) = p_node->value_len ;
		
		return 0;
	}
}

static int TravelMDHashNode( struct MDHashNode *p_node , char *travel_key_buffer , long travel_key_bufsize , long depth , void pfuncTravelProc(char *key,void *value,long value_len,void *pv) , void *pv )
{
	long		l ;
	
	int		nret ;
	
	if( p_node == NULL )
	{
		return 0;
	}
	
	if( p_node->value )
	{
		pfuncTravelProc( travel_key_buffer , p_node->value , p_node->value_len , pv );
	}
	
	for( l = 1 ; l <= 255 ; l++ )
	{
		if( p_node->a_key[l] )
		{
			if( depth > travel_key_bufsize-1 )
				return HASH_RETCODE_ERROR_BUFFER_OVERFLOW;
			
			travel_key_buffer[depth] = (char)l ;
			nret = TravelMDHashNode( p_node->a_key[l] , travel_key_buffer , travel_key_bufsize , depth + 1 , pfuncTravelProc , pv ) ;
			if( nret )
				return nret;
		}
	}

	travel_key_buffer[depth] = '\0' ;
	
	return 0;
}

int InitMDHashTree( struct MDHashTree *pmdht )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	memset( pmdht , 0x00 , sizeof(struct MDHashTree) );
	
	return 0;
}

int CleanMDHashTree( struct MDHashTree *pmdht )
{
	int		nret = 0 ;
	
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	nret = DeleteAllMDHashNode( pmdht ) ;
	if( nret )
		return nret;
	
	memset( pmdht , 0x00 , sizeof(struct MDHashTree) );
	
	return 0;
}

int PutMDHashNode( struct MDHashTree *pmdht , char *key , void *value , long value_len , BOOL (*pfuncFreeMDHashNodeProc)(void *pv) , int mode )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	return AllocMDHashNode( pmdht , & (pmdht->p_node) , key , value , value_len , pfuncFreeMDHashNodeProc , mode );
}

int GetMDHashNodePtr( struct MDHashTree *pmdht , char *key , void **pp_value , long *p_value_len )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	return QueryMDHashNode( pmdht->p_node , key , NULL , pp_value , p_value_len );
}

int DeleteMDHashNode( struct MDHashTree *pmdht , char *key )
{
	struct MDHashNode	*p_node = NULL ;
	
	int			nret ;
	
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	nret = QueryMDHashNode( pmdht->p_node , key , & p_node , NULL , NULL ) ;
	if( nret )
	{
		return nret;
	}
	
	return FreeMDHashTreeLeaf( pmdht , & p_node );
}

static int _DeleteAllMDHashNode( struct MDHashTree *pmdht , struct MDHashNode **pp_node )
{
	long			l ;
	
	if( pp_node == NULL )
	{
		if( pmdht->p_node == NULL )
		{
			return 0;
		}
		
		pp_node = & (pmdht->p_node) ;
	}
	else if( (*pp_node) == NULL )
	{
		return 0;
	}
	
	if( (*pp_node)->value )
	{
		pmdht->key_count--;
		pmdht->total_value_size -= (*pp_node)->value_len ;
		
		if( (*pp_node)->pfuncFreeMDHashNodeProc )
			(*pp_node)->pfuncFreeMDHashNodeProc( (*pp_node)->value );
		
		(*pp_node)->value = NULL ;
	}
	
	for( l = 1 ; l <= 255 ; l++ )
	{
		if( (*pp_node)->a_key[l] )
		{
			_DeleteAllMDHashNode( pmdht , (struct MDHashNode **) & ((*pp_node)->a_key[l]) );
		}
	}
	
	free(*pp_node);
	
	(*pp_node) = NULL ;
	
	return 0;
}

int DeleteAllMDHashNode( struct MDHashTree *pmdht )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	return _DeleteAllMDHashNode( pmdht , NULL );
}

int TravelMDHashTree( struct MDHashTree *pmdht , char *travel_key_buffer , long travel_key_bufsize , void pfuncTravelProc(char *key,void *value,long value_len,void *pv) , void *pv )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	memset( travel_key_buffer , 0x00 , travel_key_bufsize );
	
	return TravelMDHashNode( pmdht->p_node , travel_key_buffer , travel_key_bufsize , 0 , pfuncTravelProc , pv ) ;
}

static int _ReclaimInvalidMDHashNode( struct MDHashTree *pmdht , struct MDHashNode **pp_node )
{
	int			reclaim_flag ;
	long			l ;
	
	int			nret ;
	
	if( pp_node == NULL )
	{
		if( pmdht->p_node == NULL )
		{
			return 0;
		}
		
		pp_node = & (pmdht->p_node) ;
	}
	else if( (*pp_node) == NULL )
	{
		return 0;
	}
	
	reclaim_flag = 0 ;
	for( l = 0 ; l <= 255 ; l++ )
	{
		if( l == 0 )
		{
			if( (*pp_node)->value )
				reclaim_flag = HASH_RETCODE_INFO_NODE_EXISTED ;
		}
		else if( (*pp_node)->a_key[l] )
		{
			nret = _ReclaimInvalidMDHashNode( pmdht , (struct MDHashNode **) & ((*pp_node)->a_key[l]) ) ;
			if( nret < 0 )
				return nret;
			else if( nret > 0 )
				reclaim_flag = HASH_RETCODE_INFO_NODE_EXISTED ;
		}
	}
	if( reclaim_flag == 0 )
	{
		free(*pp_node);
		
		(*pp_node) = NULL ;
	}
	
	return reclaim_flag;
}

int ReclaimInvalidMDHashNode( struct MDHashTree *pmdht )
{
	if( pmdht == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	return _ReclaimInvalidMDHashNode( pmdht , NULL );
}

struct MDHashNode *GetMDHashRootNode( struct MDHashTree *pmdht )
{
	if( pmdht == NULL )
		return NULL;
	
	return pmdht->p_node;
}

/* ÆÕÍ¨¹þÏ£Ëã·¨*/

static int QueryHashUnitPtr( struct HashArray *pha , char *key , struct HashUnit **pp_unit , void **pp_value , long *p_value_len )
{
	unsigned long	index ;
	long		l ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	index = pha->pfuncHashExpressions( key ) % pha->prealloc_count ;
	
	for( l = 0 ; l < pha->prealloc_count ; l++ )
	{
		if( pha->p_units[index].key && strcmp( pha->p_units[index].key , key ) == 0 )
		{
			break;
		}
		/*
		if( pha->p_units[index].key ) fprintf( stderr , "hited\n" );
		*/
		
		index++;
		if( index >= pha->prealloc_count )
			index = 0 ;
	}
	if( l >= pha->prealloc_count )
	{
		return HASH_RETCODE_ERROR_KEY_NOT_EXIST;
	}
	
	if( pp_unit )
		(*pp_unit) = & (pha->p_units[index]) ;
	if( pp_value )
		(*pp_value) = pha->p_units[index].value ;
	if( p_value_len )
		(*p_value_len) = pha->p_units[index].value_len ;
	
	return 0;
}

static unsigned long funcHashExpressions_DEFAULT( char *key )
{
	unsigned long   hashval = 5381 ;
	
	while( *key )
	{
		hashval += (hashval<<5) + (*key++) ;
	}
	
	return hashval;
}

static int ConvertToNewHashArray( struct HashArray *pha , struct HashArray *new_pha )
{
	unsigned long		i , index , l ;
	struct HashUnit		*p_unit = NULL ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	for( i = 0 , p_unit = & (pha->p_units[0]); i < pha->prealloc_count ; i++ , p_unit++ )
	{
		if( p_unit->key )
		{
			index = new_pha->pfuncHashExpressions( p_unit->key ) % new_pha->prealloc_count ;
			
			for( l = 0 ; l < new_pha->prealloc_count ; l++ )
			{
				if( new_pha->p_units[index].key == NULL )
				{
					break;
				}
				else if( strcmp( new_pha->p_units[index].key , p_unit->key ) == 0 )
				{
					return HASH_RETCODE_ERROR_KEY_EXIST;
				}
				
				index++;
				if( index >= new_pha->prealloc_count )
					index = 0 ;
			}
			if( l >= new_pha->prealloc_count )
			{
				return HASH_RETCODE_ERROR_BUFFER_OVERFLOW;
			}
			
			new_pha->p_units[index].key = p_unit->key ;
			new_pha->p_units[index].value = p_unit->value ;
			new_pha->p_units[index].value_len = p_unit->value_len ;
			new_pha->p_units[index].pfuncFreeHashUnitProc = p_unit->pfuncFreeHashUnitProc ;
			
			new_pha->total_value_size += p_unit->value_len ;
			new_pha->key_count++;
		}
	}
	
	return 0;
}

int SetHashArrayEnv( struct HashArray *pha , unsigned long prealloc_count , unsigned long incremental_count , funcHashExpressions *pfuncHashExpressions )
{
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( pha->prealloc_count == 0 )
	{
		pha->p_units = (struct HashUnit *)malloc( sizeof(struct HashUnit) * prealloc_count ) ;
		if( pha->p_units == NULL )
		{
			return HASH_RETCODE_ERROR_ALLOC;
		}
		memset( pha->p_units , 0x00 , sizeof(struct HashUnit) * prealloc_count );
		
		pha->prealloc_count = prealloc_count ;
		pha->incremental_count = incremental_count ;
		
		if( pfuncHashExpressions == NULL )
			pha->pfuncHashExpressions = & funcHashExpressions_DEFAULT ;
		else
			pha->pfuncHashExpressions = pfuncHashExpressions ;
	}
	else
	{
		struct HashArray	new_ha ;
		int			nret = 0 ;
		
		/*
		printf( "realloc\n" );
		*/
		
		if( prealloc_count < 0 )
			prealloc_count = pha->prealloc_count ;
		if( incremental_count < 0 )
			incremental_count = pha->incremental_count ;
		if( pfuncHashExpressions == NULL )
			pfuncHashExpressions = & funcHashExpressions_DEFAULT ;
		
		memset( & new_ha , 0x00 , sizeof(struct HashArray) );
		
		new_ha.p_units = (struct HashUnit *)malloc( sizeof(struct HashUnit) * prealloc_count ) ;
		if( new_ha.p_units == NULL )
		{
			return HASH_RETCODE_ERROR_ALLOC;
		}
		memset( new_ha.p_units , 0x00 , sizeof(struct HashUnit) * prealloc_count );
		
		new_ha.prealloc_count = prealloc_count ;
		new_ha.incremental_count = incremental_count ;
		new_ha.pfuncHashExpressions = pfuncHashExpressions ;
		new_ha.inflate_quotiety = pha->inflate_quotiety ;
		
		nret = ConvertToNewHashArray( pha , & new_ha ) ;
		if( nret )
			return nret;
		
		free( pha->p_units );
		
		memcpy( pha , & new_ha , sizeof(struct HashArray) );
	}
	
	return 0;
}

int SetHashArrayInflateQuotiety( struct HashArray *pha , float inflate_quotiety )
{
	pha->inflate_quotiety = inflate_quotiety ;
	return 0;
}

int InitHashArray( struct HashArray *pha )
{
	int		nret = 0 ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	memset( pha , 0x00 , sizeof(struct HashArray) );
	
	nret = SetHashArrayEnv( pha , 10 , 10 , funcHashExpressions_DEFAULT ) ;
	if( nret )
		return nret;
	
	SetHashArrayInflateQuotiety( pha , 0.5 );
	
	return 0;
}

int CleanHashArray( struct HashArray *pha )
{
	int		nret = 0 ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	nret = DeleteAllHashUnit( pha ) ;
	if( nret )
		return nret;
	
	free( pha->p_units );
	
	memset( pha , 0x00 , sizeof(struct HashArray) );
	
	return nret;
}

int PutHashUnit( struct HashArray *pha , char *key , void *value , long value_len , BOOL (*pfuncFreeHashUnitProc)(void *pv) , int mode )
{
	unsigned long	index ;
	long		l ;
	int		nret = 0 ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	while( pha->key_count > (unsigned long)( (float)(pha->prealloc_count) * pha->inflate_quotiety ) )
	{
		nret = SetHashArrayEnv( pha , pha->prealloc_count + pha->incremental_count , pha->incremental_count , pha->pfuncHashExpressions ) ;
		if( nret )
			return nret;
	}
	
_PUT_AGAIN :
	
	index = pha->pfuncHashExpressions( key ) % pha->prealloc_count ;
	
	for( l = 0 ; l < pha->prealloc_count ; l++ )
	{
		if( pha->p_units[index].key == NULL )
		{
			if( ( mode & HASH_PUTMODE_REPLACE ) )
				return HASH_RETCODE_ERROR_KEY_NOT_EXIST;
			
			break;
		}
		else if( strcmp( pha->p_units[index].key , key ) == 0 )
		{
			if( ( mode & HASH_PUTMODE_ADD ) )
				return HASH_RETCODE_ERROR_KEY_EXIST;
			
			break;
		}
		
		index++;
		if( index >= pha->prealloc_count )
			index = 0 ;
	}
	if( l >= pha->prealloc_count )
	{
		nret = SetHashArrayEnv( pha , pha->prealloc_count + pha->incremental_count , pha->incremental_count , pha->pfuncHashExpressions ) ;
		if( nret )
			return nret;
		
		goto _PUT_AGAIN;
	}
	
	if( ( mode & HASH_PUTMODE_DUPLICATE ) )
	{
		char	*tmp = NULL ;
		
		tmp = (char*)malloc( value_len + 1 ) ;
		if( tmp == NULL )
			return HASH_RETCODE_ERROR_ALLOC;
		memcpy( tmp , value , value_len );
		tmp[ value_len ] = '\0' ;
		
		value = (void*)tmp ;
	}
	
	if( pha->p_units[index].key == NULL )
	{
		pha->p_units[index].key = (char*)strdup( key ) ;
		if( pha->p_units[index].key == NULL )
		{
			free( value );
			return HASH_RETCODE_ERROR_ALLOC;
		}
	}
	
	if( pha->p_units[index].value )
	{
		if( pha->p_units[index].pfuncFreeHashUnitProc )
			pha->p_units[index].pfuncFreeHashUnitProc( pha->p_units[index].value );
		
		pha->total_value_size -= pha->p_units[index].value_len ;
		pha->key_count--;
	}
	
	pha->p_units[index].value = value ;
	pha->p_units[index].value_len = value_len ;
	if( ( mode & HASH_PUTMODE_DUPLICATE ) )
		pha->p_units[index].pfuncFreeHashUnitProc = & FreeDuplicate ;
	else
		pha->p_units[index].pfuncFreeHashUnitProc = pfuncFreeHashUnitProc ;
	
	pha->total_value_size += value_len ;
	pha->key_count++;
	
	return 0;
}

int GetHashUnitPtr( struct HashArray *pha , char *key , void **pp_value , long *p_value_len )
{
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	return QueryHashUnitPtr( pha , key , NULL , pp_value , p_value_len );
}

int DeleteHashUnit( struct HashArray *pha , char *key )
{
	struct HashUnit		*p_unit = NULL ;
	int			nret = 0 ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	nret = QueryHashUnitPtr( pha , key , & p_unit , NULL , NULL ) ;
	if( nret )
		return nret;
	
	free( p_unit->key );
	p_unit->key = NULL ;
	
	if( p_unit->pfuncFreeHashUnitProc )
		p_unit->pfuncFreeHashUnitProc( p_unit->value );
	p_unit->value = NULL ;
	
	pha->total_value_size -= p_unit->value_len ;
	pha->key_count--;
	
	return 0;
}

int DeleteAllHashUnit( struct HashArray *pha )
{
	unsigned long		index ;
	struct HashUnit		*p_unit = NULL ;
	
	if( pha == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	for( index = 0 , p_unit = & (pha->p_units[0]); index < pha->prealloc_count ; index++ , p_unit++ )
	{
		if( p_unit->key )
		{
			free( p_unit->key );
			p_unit->key = NULL ;
			
			if( p_unit->pfuncFreeHashUnitProc )
				p_unit->pfuncFreeHashUnitProc( p_unit->value );
			p_unit->value = NULL ;
		}
	}
	
	pha->total_value_size = 0 ;
	pha->key_count = 0 ;
	
	return 0;
}

/* Í¨ÓÃ¹þÏ£ÈÝÆ÷ */

int InitHashContainer( struct HashContainer *phc , int algorithm )
{
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	memset( phc , 0x00 , sizeof(struct HashContainer) );
	
	if( algorithm == HASH_ALGORITHM_HASH )
	{
		phc->algorithm = HASH_ALGORITHM_HASH ;
		return InitHashArray( & (phc->container.ha) ) ;
	}
	else if( algorithm == HASH_ALGORITHM_MDHASH )
	{
		phc->algorithm = HASH_ALGORITHM_MDHASH ;
		return InitMDHashTree( & (phc->container.mdht) ) ;
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

int CleanHashContainer( struct HashContainer *phc )
{
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( phc->algorithm == HASH_ALGORITHM_HASH )
	{
		return CleanHashArray( & (phc->container.ha) ) ;
	}
	else if( phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		return CleanMDHashTree( & (phc->container.mdht) ) ;
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

int PutHashItem( struct HashContainer *phc , char *key , void *value , long value_len , BOOL (*pfuncFreeItemProc)(void *pv) , int mode )
{
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( phc->algorithm == HASH_ALGORITHM_HASH )
	{
		return PutHashUnit( & (phc->container.ha) , key , value , value_len , pfuncFreeItemProc , mode );
	}
	else if( phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		return PutMDHashNode( & (phc->container.mdht) , key , value , value_len , pfuncFreeItemProc , mode );
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

int GetHashItemPtr( struct HashContainer *phc , char *key , void **pp_value , long *p_value_len )
{
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( phc->algorithm == HASH_ALGORITHM_HASH )
	{
		return GetHashUnitPtr( & (phc->container.ha) , key , pp_value , p_value_len );
	}
	else if( phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		return GetMDHashNodePtr( & (phc->container.mdht) , key , pp_value , p_value_len );
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

int DeleteHashItem( struct HashContainer *phc , char *key )
{
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( phc->algorithm == HASH_ALGORITHM_HASH )
	{
		return DeleteHashUnit( & (phc->container.ha) , key );
	}
	else if( phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		return DeleteMDHashNode( & (phc->container.mdht) , key );
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

int DeleteAllHashItem( struct HashContainer *phc )
{
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( phc->algorithm == HASH_ALGORITHM_HASH )
	{
		return DeleteAllHashUnit( & (phc->container.ha) );
	}
	else if( phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		return DeleteAllMDHashNode( & (phc->container.mdht) );
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

void *GetHashAlgorithmObject( struct HashContainer *phc , int algorithm )
{
	if( phc == NULL )
		return NULL;
	
	if( algorithm == HASH_ALGORITHM_HASH && phc->algorithm == HASH_ALGORITHM_HASH )
	{
		return & (phc->container.ha) ;
	}
	else if( algorithm == HASH_ALGORITHM_MDHASH && phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		return & (phc->container.mdht) ;
	}
	else
	{
		return NULL;
	}
}

int TravelHashContainer( struct HashContainer *phc , char *travel_key_buffer , long travel_key_bufsize , void pfuncTravelProc(char *key,void *value,long value_len,void *pv) , void *pv )
{
	char		buffer[ 1024 + 1 ] ;
	
	if( phc == NULL )
		return HASH_RETCODE_ERROR_PARAMETER;
	
	if( phc->algorithm == HASH_ALGORITHM_MDHASH )
	{
		if( travel_key_buffer == NULL || travel_key_bufsize <= 0 )
		{
			travel_key_buffer = buffer ;
			travel_key_bufsize = sizeof(buffer) ;
		}
		
		return TravelMDHashTree( & (phc->container.mdht) , travel_key_buffer , travel_key_bufsize , pfuncTravelProc , pv );
	}
	else
	{
		return HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT;
	}
}

