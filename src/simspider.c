/*
 * simspider - Web Spider
 * author	: calvin
 * email	: calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "libsimspider.h"

funcRequestHeaderProc RequestHeaderProc ;
int RequestHeaderProc( struct DoneQueueUnit *pdqu )
{
	struct curl_slist	*curl_header_list = NULL ;
	
	curl_header_list = GetCurlHeadListPtr( pdqu ) ;
	curl_header_list = curl_slist_append( curl_header_list , "User-Agent: Mozilla/5.0(Windows NT 6.1; WOW64; rv:34.0 ) Gecko/20100101 Firefox/34.0" ) ;
	if( curl_header_list == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "curl_slist_append failed" );
		return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
	}
	FreeCurlHeadList1Later( pdqu , curl_header_list );
	
	return 0;
}

funcResponseBodyProc ResponseBodyProc ;
int ResponseBodyProc( struct DoneQueueUnit *pdqu )
{
	struct SimSpiderBuf	*buf = NULL ;
	
	buf = GetDoneQueueUnitBodyBuffer(pdqu) ;
	DebugLog( __FILE__ , __LINE__ , "[%s] HTTP BODY [%.*s]" , GetDoneQueueUnitUrl(pdqu) , (int)(buf->len) , buf->base );
	
	return 0;
}

funcFinishTaskProc FinishTaskProc ;
int FinishTaskProc( struct DoneQueueUnit *pdqu )
{
	int			*p_count = NULL ;
	
	printf( ">>> [%3d] [%2ld] [%2ld] [%s] [%s]\n" , GetDoneQueueUnitStatus(pdqu) , GetDoneQueueUnitRecursiveDepth(pdqu) , GetDoneQueueUnitRetryCount(pdqu)
		 , GetDoneQueueUnitRefererUrl(pdqu) , GetDoneQueueUnitUrl(pdqu) );
	
	p_count = GetSimSpiderPublicData( GetSimSpiderEnv(pdqu) ) ;
	(*p_count)++;
	
	return 0;
}

int simspider( char *url , long max_concurrent_count )
{
	struct SimSpiderEnv	*penv = NULL ;
	int			count ;
	
	int			nret = 0 ;
	
	nret = InitSimSpiderEnv( & penv , "simspider.log" ) ;
	if( nret )
	{
		printf( "InitSimSpiderEnv failed[%d]\n" , nret );
		return 1;
	}
	
	ResizeRequestQueue( penv , 10*1024*1024 );
	SetCertificateFilename( penv , "server.crt" );
	SetMaxConcurrentCount( penv , max_concurrent_count );
	SetMaxRetryCount( penv , 10 );
	
	count = 0 ;
	SetSimSpiderPublicData( penv , (void*) & count );
	SetRequestHeaderProc( penv , & RequestHeaderProc );
	SetResponseBodyProc( penv , & ResponseBodyProc );
	SetFinishTaskProc( penv , & FinishTaskProc );
	
	nret = SimSpiderGo( penv , "" , url ) ;
	
	CleanSimSpiderEnv( & penv );
	
	printf( "Total [%d]urls\n" , count );
	
	return -nret;
}

static void usage()
{
	printf( "simspider v%s\n" , __SIMSPIDER_VERSION );
	printf( "copyright by calvin<calvinwilliams.c@gmail.com> 2014,2015\n" );
	printf( "USAGE : simspider url [ max_concurrent_count ]\n" );
}

int main( int argc , char *argv[] )
{
	setbuf( stdout , NULL );
	
	if( argc == 1 )
	{
		usage();
		return 0;
	}
	else if( argc == 1 + 1 )
	{
		return -simspider( argv[1] , 1 );
	}
	else if( argc == 1 + 2 )
	{
		return -simspider( argv[1] , atol(argv[2]) );
	}
	else
	{
		usage();
		return 7;
	}
}

