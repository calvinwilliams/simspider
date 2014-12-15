#include "libsimspider.h"

#define ENTRY_URL	"http://www.oschina.net/"

funcTravelDoneQueueProc TravelDoneQueueProc ;
void TravelDoneQueueProc( char *key , void *value , long value_len , void *pv )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)value ;
	
	printf( "[%4d] [%2ld] [%s]\n" , GetDoneQueueUnitStatus(pdqu) , GetDoneQueueUnitRecursiveDepth(pdqu) , GetDoneQueueUnitUrl(pdqu) );
	
	return;
}

funcResponseBodyProc ResponseBodyProc ;
int ResponseBodyProc( char *url , char *data , long *p_data_len )
{
	char	*ptr = NULL ;
	
	ptr = strstr( data , "title\xEF\xBC\x9D" ) ;
	if( ptr )
	{
		memcpy( ptr , "title=  " , 8 );
	}
	
	return SIMSPIDER_INFO_THEN_DO_IT_FOR_DEFAULT;
}

int www_oschina_net()
{
	struct SimSpiderEnv	*penv = NULL ;
	
	char			*entry_urls[] = { ENTRY_URL , NULL } ;
	
	int			nret = 0 ;
	
	nret = InitSimSpiderEnv( & penv , "www.oschina.net.log" ) ;
	if( nret )
	{
		printf( "InitSimSpiderEnv failed[%d]\n" , nret );
		return 1;
	}
	
	SetValidFileExtnameSet( penv , "html php jsp asp cgi fcgi" );
	AllowEmptyFileExtname( penv , 1 );
	SetMaxRecursiveDepth( penv , 2 );
	
	SetResponseBodyProc( penv , & ResponseBodyProc );
	SetTravelDoneQueueProc( penv , & TravelDoneQueueProc );
	
	nret = SimSpiderGo( penv , entry_urls , NULL , NULL );
	
	CleanSimSpiderEnv( & penv );
	
	return 0;
}

int main( int argc , char *argv[] )
{
	return -www_oschina_net();
}

