/*
 * libsimspider-queue - Web Spider Engine Queue Library
 * author	: calvin
 * email	: calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "libsimspider-queue.h"

funcResetRequestQueueProc ResetRequestQueueProc_DEFAULT ;
int ResetRequestQueueProc_DEFAULT( struct SimSpiderEnv *penv )
{
	CleanMemoryQueue( GetRequestQueueHandler(penv) );
	return 0;
}

funcResizeRequestQueueProc ResizeRequestQueueProc_DEFAULT ;
int ResizeRequestQueueProc_DEFAULT( struct SimSpiderEnv *penv , long new_size )
{
	struct MemoryQueue	*pmq = NULL ;
	
	int			nret = 0 ;
	
	pmq = GetRequestQueueHandler(penv) ;
	
	nret = DestroyMemoryQueue( & pmq ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "DestroyMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_MEMQUEUE;
	}
	
	pmq = NULL ;
	nret = CreateMemoryQueue( & pmq , new_size , -1 , -1 ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "CreateMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_MEMQUEUE;
	}
	
	SetRequestQueueHandler( penv , pmq );
	
	return 0;
}

static funcPushRequestQueueUnitProc PushRequestQueueUnitProc_DEFAULT ;
int PushRequestQueueUnitProc_DEFAULT( struct SimSpiderEnv *penv , char *url )
{
	int		nret = 0 ;
	
	nret = AddQueueBlock( GetRequestQueueHandler(penv) , url , strlen(url) + 1 , NULL ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] failed[%d]" , url , nret );
		return SIMSPIDER_ERROR_REQUEST_QUEUE_OVERFLOW;
	}
	else
	{
		DebugLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] ok" , url );
	}
	
	return 0;
}

static funcPopupRequestQueueUnitProc PopupRequestQueueUnitProc_DEFAULT ;
int PopupRequestQueueUnitProc_DEFAULT( struct SimSpiderEnv *penv , char url[SIMSPIDER_MAXLEN_URL+1] )
{
	struct QueueBlock	*pqb = NULL ;
	
	int			nret = 0 ;
	
	nret = TravelQueueBlockByOrder( GetRequestQueueHandler(penv) , & pqb ) ;
	if( nret == MEMQUEUE_WARN_NO_BLOCK )
	{
		DebugLog( __FILE__ , __LINE__ , "TravelQueueBlockByOrder ok , but no data" );
		return SIMSPIDER_INFO_NO_TASK_IN_REQUEST_QUEUE;
	}
	else if( nret < 0 )
	{
		ErrorLog( __FILE__ , __LINE__ , "TravelQueueBlockByOrder failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_MEMQUEUE;
	}
	else
	{
		strcpy( url , (char*)pqb + sizeof(struct QueueBlock) );
		
		RemoveQueueBlock( GetRequestQueueHandler(penv) , pqb );
		DebugLog( __FILE__ , __LINE__ , "RemoveQueueBlock[%s] ok" , url );
	}
	
	return 0;
}

static funcResetDoneQueueProc ResetDoneQueueProc_DEFAULT ;
int ResetDoneQueueProc_DEFAULT( struct SimSpiderEnv *penv )
{
	struct HashContainer		*done_queue = NULL ;
	
	done_queue = GetDoneQueueHandler( penv ) ;
	
	DeleteAllHashItem( done_queue );
	
	return 0;
}

static funcResizeDoneQueueProc ResizeDoneQueueProc_DEFAULT ;
int ResizeDoneQueueProc_DEFAULT( struct SimSpiderEnv *penv , long new_size )
{
	return 0;
}

static funcQueryDoneQueueUnitProc QueryDoneQueueUnitProc_DEFAULT ;
int QueryDoneQueueUnitProc_DEFAULT( struct SimSpiderEnv *penv , char url[SIMSPIDER_MAXLEN_URL+1] , struct DoneQueueUnit *pdqu , int SizeOfDoneQueueUnit )
{
	void		*value = NULL ;
	
	int		nret = 0 ;
	
	nret = GetHashItemPtr( GetDoneQueueHandler(penv) , (unsigned char *) url , & value , NULL ) ;
	if( nret == HASH_RETCODE_ERROR_KEY_NOT_EXIST )
	{
		return SIMSPIDER_INFO_NO_TASK_IN_DONE_QUEUE;
	}
	else if( nret < 0 )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr[%s] failed[%d] errno[%d]" , url , nret , errno );
		return SIMSPIDER_ERROR_LIB_HASHX;
	}
	else
	{
		if( pdqu )
		{
			memcpy( pdqu , (char*) value , SizeOfDoneQueueUnit );
		}
		return 0;
	}
}

static BOOL FreeeDoneQueueUnit( void *pv )
{
	FreeDoneQueueUnit( pv );
	
	return TRUE;
}

static funcAddDoneQueueUnitProc AddDoneQueueUnitProc_DEFAULT ;
int AddDoneQueueUnitProc_DEFAULT( struct SimSpiderEnv *penv , char *referer_url , char *url , long recursive_depth , int SizeOfDoneQueueUnit )
{
	struct DoneQueueUnit	*pdqu = NULL ;
	
	int			nret = 0 ;
	
	nret = QueryDoneQueueUnitProc_DEFAULT( penv , url , NULL , 0 ) ;
	if( nret == SIMSPIDER_INFO_NO_TASK_IN_DONE_QUEUE )
	{
		pdqu = AllocDoneQueueUnit( penv , referer_url , url , recursive_depth ) ;
		if( pdqu == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "AllocDoneQueueUnit failed errno[%d]" , errno );
			return SIMSPIDER_ERROR_LIB_HASHX;
		}
		
		nret = PutHashItem( GetDoneQueueHandler(penv) , (unsigned char *) url , (void *) pdqu , SizeOfDoneQueueUnit , & FreeeDoneQueueUnit , HASH_PUTMODE_ADD ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "PutHashItem failed[%d] errno[%d]" , nret , errno );
			FreeeDoneQueueUnit( pdqu );
			return SIMSPIDER_ERROR_LIB_HASHX;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "PutHashItem[%s] ok" , url );
		}
		
		return SIMSPIDER_INFO_ADD_TASK_IN_DONE_QUEUE;
	}
	else if( nret )
	{
		return SIMSPIDER_ERROR_LIB_HASHX;
	}
	else
	{
		return 0;
	}
}

static funcUpdateDoneQueueUnitProc UpdateDoneQueueUnitProc_DEFAULT ;
int UpdateDoneQueueUnitProc_DEFAULT( struct SimSpiderEnv *penv , struct DoneQueueUnit *pdqu , int SizeOfDoneQueueUnit )
{
	void		*value = NULL ;
	
	int		nret = 0 ;
	
	nret = GetHashItemPtr( GetDoneQueueHandler(penv) , (unsigned char *)GetDoneQueueUnitUrl(pdqu) , & value , NULL ) ;
	if( nret < 0 )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr[%s] failed[%d] errno[%d]" , GetDoneQueueUnitUrl(pdqu) , nret , errno );
		return SIMSPIDER_ERROR_LIB_HASHX;
	}
	else
	{
		memcpy( (char*)value , pdqu , SizeOfDoneQueueUnit );
		return 0;
	}
}

int BindDefaultRequestQueueHandler( struct SimSpiderEnv *penv )
{
	struct MemoryQueue	*pmq = NULL ;
	
	int			nret = 0 ;
	
	nret = CreateMemoryQueue( & pmq , SIMSPIDER_DEFAULT_REQUESTQUEUE_SIZE , -1 , -1 ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "CreateMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return -1;
	}
	
	SetRequestQueueHandler( penv , pmq );
	
	SetResetRequestQueueProc( penv , & ResetRequestQueueProc_DEFAULT );
	SetResizeRequestQueueProc( penv , & ResizeRequestQueueProc_DEFAULT );
	SetPushRequestQueueUnitProc( penv , & PushRequestQueueUnitProc_DEFAULT );
	SetPopupRequestQueueUnitProc( penv , & PopupRequestQueueUnitProc_DEFAULT );
	
	return 0;
}

void UnbindDefaultRequestQueueHandler( struct SimSpiderEnv *penv )
{
	struct MemoryQueue		*request_queue = NULL ;
	
	request_queue = GetRequestQueueHandler( penv ) ;
	
	DestroyMemoryQueue( & request_queue );
	
	SetRequestQueueHandler( penv , NULL );
	
	SetResetRequestQueueProc( penv , NULL );
	SetResizeRequestQueueProc( penv , NULL );
	SetPushRequestQueueUnitProc( penv , NULL );
	SetPopupRequestQueueUnitProc( penv , NULL );
	
	return;
}

int BindDefaultDoneQueueHandler( struct SimSpiderEnv *penv )
{
	struct HashContainer		*phc = NULL ;
	
	int				nret = 0 ;
	
	phc = (struct HashContainer *)malloc( sizeof(struct HashContainer) ) ;
	if( phc == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "alloc failed , errno[%d]" , errno );
		return -1;
	}
	memset( phc , 0x00 , sizeof(struct HashContainer) );
	
	nret = InitHashContainer( phc , HASH_ALGORITHM_MDHASH ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "InitHashContainer failed[%d] errno[%d]" , nret , errno );
		return -1;
	}
	
	SetDoneQueueHandler( penv , phc );
	
	SetResetDoneQueueProc( penv , & ResetDoneQueueProc_DEFAULT );
	SetResizeDoneQueueProc( penv , & ResizeDoneQueueProc_DEFAULT );
	SetQueryDoneQueueUnitProc( penv , & QueryDoneQueueUnitProc_DEFAULT );
	SetAddDoneQueueUnitProc( penv , & AddDoneQueueUnitProc_DEFAULT );
	SetUpdateDoneQueueUnitProc( penv , & UpdateDoneQueueUnitProc_DEFAULT );
	
	return 0;
}

void UnbindDefaultDoneQueueHandler( struct SimSpiderEnv *penv )
{
	struct HashContainer		*done_queue = NULL ;
	
	done_queue = GetDoneQueueHandler( penv ) ;
	
	CleanHashContainer( done_queue );
	
	free( done_queue );
	
	SetRequestQueueHandler( penv , NULL );
	
	SetResetDoneQueueProc( penv , NULL );
	SetResizeDoneQueueProc( penv , NULL );
	SetQueryDoneQueueUnitProc( penv , NULL );
	SetAddDoneQueueUnitProc( penv , NULL );
	SetUpdateDoneQueueUnitProc( penv , NULL );
	
	return;
}
