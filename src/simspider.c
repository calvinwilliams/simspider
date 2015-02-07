/*
 * simspider - Net Spider Engine
 * author	: calvin
 * email	: calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "libsimspider.h"

funcRequestHeaderProc RequestHeaderProc ;
int RequestHeaderProc( struct DoneQueueUnit *pdqu )
{
	CURL			*curl = NULL ;
	struct curl_slist	*header_list = NULL ;
	char			buffer[ 1024 + 1 ] ;
	
	curl = GetDoneQueueUnitCurl(pdqu) ;
	
	header_list = curl_slist_append( header_list , "User-Agent: Mozilla/5.0(Windows NT 6.1; WOW64; rv:34.0 ) Gecko/20100101 Firefox/34.0" ) ;
	
	if( GetDoneQueueUnitRefererUrl(pdqu) )
	{
		memset( buffer , 0x00 , sizeof(buffer) );
		SNPRINTF( buffer , sizeof(buffer) , "Referer: %s" , GetDoneQueueUnitRefererUrl(pdqu) );
		header_list = curl_slist_append( header_list , buffer ) ;
	}
	
	curl_easy_setopt( curl , CURLOPT_HTTPHEADER , header_list );
	FreeCurlList1Later( pdqu , header_list );
	
	return 0;
}

funcResponseBodyProc ResponseBodyProc ;
int ResponseBodyProc( struct DoneQueueUnit *pdqu )
{
	printf( ">>> [%s]\n" , GetDoneQueueUnitUrl(pdqu) );

	return 0;
}

funcTravelDoneQueueProc TravelDoneQueueProc ;
void TravelDoneQueueProc( char *key , void *value , long value_len , void *pv )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)value ;
	
	printf( "[%5d] [%2ld] [%s] [%s]\n" , GetDoneQueueUnitStatus(pdqu) , GetDoneQueueUnitRecursiveDepth(pdqu) , GetDoneQueueUnitRefererUrl(pdqu) , GetDoneQueueUnitUrl(pdqu) );
	
	return;
}

int simspider( char *entry_url , long max_concurrent_count )
{
	struct SimSpiderEnv	*penv = NULL ;
	
	int			nret = 0 ;
	
	nret = InitSimSpiderEnv( & penv , "simspider.log" ) ;
	if( nret )
	{
		printf( "InitSimSpiderEnv failed[%d]\n" , nret );
		return 1;
	}
	
	AllowEmptyFileExtname( penv , 1 );
	SetCertificateFilename( penv , "../cert/server.crt" );
	SetMaxConcurrentCount( penv , max_concurrent_count );
	SetRequestHeaderProc( penv , & RequestHeaderProc );
	SetResponseBodyProc( penv , & ResponseBodyProc );
	SetTravelDoneQueueProc( penv , & TravelDoneQueueProc );
	
	nret = SimSpiderGo( penv , entry_url ) ;
	
	CleanSimSpiderEnv( & penv );
	
	return 0;
}

static void usage()
{
	printf( "simspider v%s\n" , __SIMSPIDER_VERSION );
	printf( "copyright by calvin<calvinwilliams.c@gmail.com> 2014,2015\n" );
	printf( "USAGE : simspider url [ max_concurrent_count ]\n" );
}

int main( int argc , char *argv[] )
{
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

