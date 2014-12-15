#include "libsimspider.h"

/*
for test
./simspider http://192.168.2.104/
./simspider https://192.168.2.182/
*/

funcTravelDoneQueueProc TravelDoneQueueProc ;
void TravelDoneQueueProc( char *key , void *value , long value_len , void *pv )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)value ;
	
	printf( "[%4d] [%2ld] [%s]\n" , GetDoneQueueUnitStatus(pdqu) , GetDoneQueueUnitRecursiveDepth(pdqu) , GetDoneQueueUnitUrl(pdqu) );
	
	return;
}

int simspider( char *url )
{
	struct SimSpiderEnv	*penv = NULL ;
	
	char			*entry_urls[] = { url , NULL } ;
	
	int			nret = 0 ;
	
	nret = InitSimSpiderEnv( & penv , NULL ) ;
	if( nret )
	{
		printf( "InitSimSpiderEnv failed[%d]\n" , nret );
		return 1;
	}
	
	SetValidFileExtnameSet( penv , "html php jsp asp cgi fcgi" );
	AllowEmptyFileExtname( penv , 1 );
	SetCertificateFilename( penv , "../cert/server.crt" );
	
	SetTravelDoneQueueProc( penv , & TravelDoneQueueProc );
	
	nret = SimSpiderGo( penv , entry_urls , NULL , NULL );
	
	CleanSimSpiderEnv( & penv );
	
	return 0;
}

static void usage()
{
	printf( "simspider v%s\n" , __SIMSPIDER_VERSION );
	printf( "copyright by calvin<calvinwilliams.c@gmail.com> 2014,2015\n" );
	printf( "USAGE : simspider url\n" );
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
		return -simspider( argv[1] );
	}
	else
	{
		usage();
		return 7;
	}
}

