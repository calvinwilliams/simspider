#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "curl/curl.h"

#include "fasterxml.h"

#include "memque.h"
#include "HashX.h"
#include "LOGC.h"
#include "libsimspider.h"

char	__SIMSPIDER_VERSION_1_0_0[] = "1.0.0" ;
char	*__SIMSPIDER_VERSION = __SIMSPIDER_VERSION_1_0_0 ;

struct StringBuffer
{
	long			bufsize ;
	long			len ;
	char			*base ;
} ;

struct SimSpiderEnv
{
	char			url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
	
	struct StringBuffer	header ;
	struct StringBuffer	body ;
	
	char			valid_file_extname_set[ SIMSPIDER_VALID_FILE_EXTNAME_SET + 1 ] ;
	int			allow_empty_file_extname ;
	char			cert_pathfilename[ SIMSPIDER_MAXLEN_FILENAME + 1 ] ;
	int			allow_runoutof_website ;
	long			max_recursive_depth ;
	long			request_delay ;
	funcResponseHeaderProc	*pfuncResponseHeaderProc ;
	funcResponseBodyProc	*pfuncResponseBodyProc ;
	funcParserHtmlNodeProc	*pfuncParserHtmlNodeProc ;
	funcTravelDoneQueueProc	*pfuncTravelDoneQueueProc ;
	
	struct MemoryQueue	*request_queue ;
	struct HashContainer	done_queue ;
	
	struct DoneQueueUnit	*pdqu ;
	
	void			*p ;
} ;

struct DoneQueueUnit
{
	char			*url ;
	char			*custom_header ;
	char			*post_data ;
	int			status ;
	long			recursive_depth ;
} ;

static BOOL FreeDoneQueueUnit( void *pv )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)pv ;
	
	if( pdqu )
	{
		if( pdqu->url )
			free( pdqu->url );
		if( pdqu->custom_header )
			free( pdqu->custom_header );
		if( pdqu->post_data )
			free( pdqu->post_data );
		free( pdqu );
	}
	
	return TRUE;
}

static struct DoneQueueUnit *AllocDoneQueueUnit( char *url , char *custom_header , char *post_data , int status , long recursive_depth )
{
	struct DoneQueueUnit	*pdqu = NULL ;
	
	pdqu = (struct DoneQueueUnit *)malloc( sizeof(struct DoneQueueUnit) ) ;
	if( pdqu == NULL )
		return NULL;
	memset( pdqu , 0x00 , sizeof(struct DoneQueueUnit) );
	
	pdqu->url = strdup( url ) ;
	if( pdqu->url == NULL )
	{
		FreeDoneQueueUnit( pdqu );
		return NULL;
	}
	
	if( custom_header )
	{
		pdqu->custom_header = strdup( custom_header ) ;
		if( pdqu->custom_header == NULL )
		{
			FreeDoneQueueUnit( pdqu );
			return NULL;
		}
	}
	
	if( post_data )
	{
		pdqu->post_data = strdup( post_data ) ;
		if( pdqu->post_data == NULL )
		{
			FreeDoneQueueUnit( pdqu );
			return NULL;
		}
	}
	
	pdqu->status = status ;
	pdqu->recursive_depth = recursive_depth ;
	
	return pdqu;
}

void SetEnvCustomDataPtr( void *_penv , void *p )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *)_penv ;
	penv->p = p ;
	return;
}

void *GetEnvCustomDataPtr( void *_penv )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *)_penv ;
	return penv->p;
}

char *GetDoneQueueUnitUrl( void *_pdqu )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)_pdqu ;
	return pdqu->url;
}

int GetDoneQueueUnitStatus( void *_pdqu )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)_pdqu ;
	return pdqu->status;
}

long GetDoneQueueUnitRecursiveDepth( void *_pdqu )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)_pdqu ;
	return pdqu->recursive_depth;
}

static struct SimSpiderEnv *AllocSimSpiderEnv( size_t header_bufsize , size_t body_bufsize )
{
	struct SimSpiderEnv	*penv = NULL ;
	
	penv = (struct SimSpiderEnv *)malloc( sizeof(struct SimSpiderEnv) ) ;
	if( penv == NULL )
		return NULL;
	memset( penv , 0x00 , sizeof(struct SimSpiderEnv) );
	
	penv->header.base = (char*)malloc( header_bufsize ) ;
	if( penv->header.base == NULL )
		return NULL;
	memset( penv->header.base , 0x00 , header_bufsize );
	penv->header.bufsize = header_bufsize ;
	penv->header.len = 0 ;
	
	penv->body.base = (char*)malloc( body_bufsize ) ;
	if( penv->body.base == NULL )
		return NULL;
	memset( penv->body.base , 0x00 , body_bufsize );
	penv->body.bufsize = body_bufsize ;
	penv->body.len = 0 ;
	
	return penv;
}

static int ReallocHeaderBuffer( struct SimSpiderEnv *penv , size_t new_bufsize )
{
	char	*new_base = NULL ;
	
	if( new_bufsize <= penv->header.bufsize )
		return -2;
	
	new_base = (char*)realloc( penv->header.base , new_bufsize ) ;
	if( new_base == NULL )
		return -1;
	memset( new_base + penv->header.len , 0x00 , new_bufsize - penv->header.len );
	
	penv->header.base = new_base ;
	penv->header.bufsize = new_bufsize ;
	
	return 0;
}

static int ReallocBodyBuffer( struct SimSpiderEnv *penv , size_t new_bufsize )
{
	char	*new_base = NULL ;
	
	if( new_bufsize <= penv->body.bufsize )
		return -2;
	
	new_base = (char*)realloc( penv->body.base , new_bufsize ) ;
	if( new_base == NULL )
		return -1;
	memset( new_base + penv->body.len , 0x00 , new_bufsize - penv->body.len );
	
	penv->body.base = new_base ;
	penv->body.bufsize = new_bufsize ;
	
	return 0;
}

static int CleanSimSpiderBuffer( struct SimSpiderEnv *penv )
{
	memset( penv->header.base , 0x00 , penv->header.bufsize );
	penv->header.len = 0 ;
	memset( penv->body.base , 0x00 , penv->body.bufsize );
	penv->body.len = 0 ;
	return 0;
}

static void FreeSimSpiderEnv( struct SimSpiderEnv *penv )
{
	free( penv->body.base );
	memset( penv , 0x00 , sizeof(struct SimSpiderEnv) );
	free( penv );
	return;
}

int InitSimSpiderEnv( struct SimSpiderEnv **ppenv , char *log_file_format , ... )
{
	int		nret = 0 ;
	
	if( log_file_format )
	{
		va_list         valist ;
		va_start( valist , log_file_format );
		SetLogFileV( log_file_format , valist );
		va_end( valist );
	}
	else if( getenv("SIMSPIDER_LOGFILE") )
	{
		SetLogFile( "%s" , getenv("SIMSPIDER_LOGFILE") );
	}
	else if( getenv("SIMSPIDER_LOGDIR") )
	{
		SetLogFile( "%s/simspider.log" , getenv("SIMSPIDER_LOGDIR") );
	}
	
	if( getenv("SIMSPIDER_LOGLEVEL") )
	{
		if( STRCMP( getenv("SIMSPIDER_LOGLEVEL") , == , "DEBUG" ) )
			SetLogLevel( LOGLEVEL_DEBUG );
		else if( STRCMP( getenv("SIMSPIDER_LOGLEVEL") , == , "INFO" ) )
			SetLogLevel( LOGLEVEL_INFO );
		else if( STRCMP( getenv("SIMSPIDER_LOGLEVEL") , == , "WARN" ) )
			SetLogLevel( LOGLEVEL_WARN );
		else if( STRCMP( getenv("SIMSPIDER_LOGLEVEL") , == , "ERROR" ) )
			SetLogLevel( LOGLEVEL_ERROR );
		else if( STRCMP( getenv("SIMSPIDER_LOGLEVEL") , == , "FATAL" ) )
			SetLogLevel( LOGLEVEL_FATAL );
		else
			return SIMSPIDER_ERROR_LIB_LOGC;
	}
	else
	{
		SetLogLevel( LOGLEVEL_DEBUG );
	}
	
	(*ppenv) = AllocSimSpiderEnv( 4096+1 , 4096+1 ) ;
	if( (*ppenv) == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "AllocSimSpiderEnv failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_ALLOC;
	}
	
	nret = CreateMemoryQueue( & ((*ppenv)->request_queue) , 10*1024*1024 , -1 , -1 ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "CreateMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_MEMQUEUE;
	}
	
	nret = InitHashContainer( & ((*ppenv)->done_queue) , HASH_ALGORITHM_MDHASH ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "CreateMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_HASHX;
	}
	
	SetValidFileExtnameSet( (*ppenv) , SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION );
	AllowEmptyFileExtname( (*ppenv) , 0 );
	AllowRunOutofWebsite( (*ppenv) , 0 );
	
	curl_global_init( CURL_GLOBAL_ALL );
	
	AddSkipXmlTag( "meta" );
	AddSkipXmlTag( "span" );
	AddSkipXmlTag( "br" );
	AddSkipXmlTag( "p" );
	AddSkipXmlTag( "img" );
	AddSkipXmlTag( "link" );
	
	return 0;
}

void CleanSimSpiderEnv( struct SimSpiderEnv **ppenv )
{
	DestroyMemoryQueue( & ((*ppenv)->request_queue) );
	CleanHashContainer( & ((*ppenv)->done_queue) );
	
	FreeSimSpiderEnv( (*ppenv) );
	
	CleanSkipXmlTags();
	
	curl_global_cleanup();
	
	return;
}

void ResetSimSpiderEnv( struct SimSpiderEnv *penv )
{
	CleanMemoryQueue( penv->request_queue );
	DeleteAllHashItem( & (penv->done_queue) );
	
	return;
}

void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set )
{
	memset( penv->valid_file_extname_set , 0x00 , sizeof(penv->valid_file_extname_set) );
	snprintf( penv->valid_file_extname_set , sizeof(penv->valid_file_extname_set)-1 , " %s " , valid_file_extname_set );
	return;
}

void AllowEmptyFileExtname( struct SimSpiderEnv *penv , int allow_empty_file_extname )
{
	penv->allow_empty_file_extname = allow_empty_file_extname ;
	return;
}

void AllowRunOutofWebsite( struct SimSpiderEnv *penv , int allow_runoutof_website )
{
	penv->allow_runoutof_website = allow_runoutof_website ;
	return;
}

void SetMaxRecursiveDepth( struct SimSpiderEnv *penv , long max_recursive_depth )
{
	penv->max_recursive_depth = max_recursive_depth ;
	return;
}

void SetCertificateFilename( struct SimSpiderEnv *penv , char *cert_pathfilename_format , ... )
{
	va_list		valist ;
	
	va_start( valist , cert_pathfilename_format );
	memset( penv->cert_pathfilename , 0x00 , sizeof(penv->cert_pathfilename) );
	vsnprintf( penv->cert_pathfilename , sizeof(penv->cert_pathfilename)-1 , cert_pathfilename_format , valist );
	va_end( valist );
	
	return;
}

void SetRequestDelay( struct SimSpiderEnv *penv , long seconds )
{
	penv->request_delay = seconds ;
	return;
}

void SetResponseHeaderProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseHeaderProc )
{
	penv->pfuncResponseHeaderProc = pfuncResponseHeaderProc ;
	return;
}

void SetResponseBodyProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseBodyProc )
{
	penv->pfuncResponseBodyProc = pfuncResponseBodyProc ;
	return;
}

void SetParserHtmlNodeProc( struct SimSpiderEnv *penv , funcParserHtmlNodeProc *pfuncParserHtmlNodeProc )
{
	penv->pfuncParserHtmlNodeProc = pfuncParserHtmlNodeProc ;
	return;
}

void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc )
{
	penv->pfuncTravelDoneQueueProc = pfuncTravelDoneQueueProc ;
	return;
}

static char *strrnchr( char *str , int str_len , int c)
{
	char		*ptr = NULL ;
	for( ptr = str + str_len - 1 ; ptr >= str ; ptr-- )
	{
		if( (*ptr) == (char)c )
			return ptr;
	}
	
	return NULL;
}

static char *strprbrk( char *str , char *ct )
{
	char		*ptr = NULL ;
	for( ptr = str + strlen(str) - 1 ; ptr >= str ; ptr-- )
	{
		if( strchr( ct , (*ptr) ) )
			return ptr;
	}
	
	return NULL;
}

static char *strprnbrk( char *str , int str_len , char *ct )
{
	char		*ptr = NULL ;
	for( ptr = str + str_len - 1 ; ptr >= str ; ptr-- )
	{
		if( strchr( ct , (*ptr) ) )
			return ptr;
	}
	
	return NULL;
}

static int FormatNewUrl( struct SimSpiderEnv *penv , char *propvalue , int propvalue_len , char *url )
{
	char		*ptr = NULL ;
	char		*ptr2 = NULL ;
	
	if( STRNCMP( propvalue , == , "http" , 4 ) )
	{
		/*
		http://A/B
		http://C/D
		*/
		ptr = strchr( url , '/' ) ;
		if( ptr == NULL )
			return 0;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 0;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 0;
		if( STRNCMP( propvalue , != , url , ptr+1-url ) && penv->allow_runoutof_website == 0 )
			return -1;
		strncpy( url , propvalue , propvalue_len );
		url[propvalue_len] = '\0' ;
		return 0;
	}
	else if( propvalue[0] == '/' )
	{
		/*
		http://A/B
		        /C
		*/
		ptr = strchr( url , '/' ) ;
		if( ptr == NULL )
			return 0;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 0;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 0;
		snprintf( ptr+1 , sizeof(url)-(ptr-url) , "%.*s" , propvalue_len , propvalue );
	}
	else
	{
		/*
		http://A/B/C.html
		           D.html
		http://A/B.html
		        /D.html
		http://A/B
		          /C.html
		*/
		ptr = strprbrk( url , "/.?" ) ;
		if( ptr == NULL )
		{
			return -2;
		}
		else if( (*ptr) == '/' )
		{
			snprintf( url + strlen(url) , sizeof(url) - strlen(url) - 1 , "%.*s" , propvalue_len , propvalue );
		}
		else
		{
			ptr2 = strrnchr( url , ptr - url , '/' ) ;
			if( (*ptr) == '.' && ptr2 && url + 2 <= ptr2 && MEMCMP( ptr2 - 2 , == , ":/" , 2 ) )
			{
				snprintf( url + strlen(url) , sizeof(url) - strlen(url) - 1 , "/%.*s" , propvalue_len , propvalue );
			}
			else
			{
				ptr = strrnchr( url , ptr - url , '/' ) ;
				if( ptr == NULL )
					return 0;
				snprintf( ptr+1 , sizeof(url)-(ptr-url) , "%.*s" , propvalue_len , propvalue );
			}
		}
	}
	
	while(1)
	{
		ptr = strstr( url , "/./" ) ;
		if( ptr == NULL )
			break;
		memmove( ptr+1 , ptr+3 , strlen(ptr+3)+1 );
	}
	
	while(1)
	{
		ptr = strstr( url , "/../" ) ;
		if( ptr == NULL )
			break;
		(*ptr) = '\0' ;
		ptr2 = strrchr( url , '/' ) ;
		if( ptr2 == NULL )
			break;
		memmove( ptr2+1 , ptr+4 , strlen(ptr+4)+1 );
	}
	
	return 0;
}

static int CheckHttpProtocol( char *propvalue )
{
	char	*ptr = NULL ;
	int	len ;
	
	ptr = strstr( propvalue , "://" ) ;
	if( ptr == NULL )
		return 1;
	
	len = ptr - propvalue ;
	if( len == 4 && STRNICMP( propvalue , == , "HTTP" , len ) )
		return 1;
	else if( len == 5 && STRNICMP( propvalue , == , "HTTPS" , len ) )
		return 1;
	else
		return 0;
}

static int CheckFileExtname( struct SimSpiderEnv *penv , char *propvalue , int propvalue_len )
{
	int			param_flag ;
	char			*end = NULL , *begin = NULL ;
	char			file_extname[ 64 + 1 ] ;
	
	if( *(propvalue+propvalue_len-1) == '/' )
		return 1;
	
	/*
	http://.../A.B
		    begin
		      end
	http://.../A.B?C...
		    begin
		      end
	A
	 begin
	 end
	*/
	end = strrnchr( propvalue , propvalue_len , '?' ) ;
	if( end )
	{
		param_flag = 1 ;
	}
	else
	{
		param_flag = 0 ;
		end = propvalue + propvalue_len ;
	}
	
	begin = strprnbrk( propvalue , end - propvalue , "/." ) ;
	if( begin )
	{
		if( (*begin) == '/' )
		{
			if( param_flag == 1 && penv->allow_empty_file_extname == 0 )
				return 0;
			else
				return 1;
		}
		begin++;
	}
	else
	{
		if( param_flag == 1 && penv->allow_empty_file_extname == 0 )
			return 0;
		else
			return 1;
	}
	
	memset( file_extname , 0x00 , sizeof(file_extname) );
	snprintf( file_extname , sizeof(file_extname) , " %.*s " , end - begin , begin );
	if( strstr( penv->valid_file_extname_set , file_extname ) )
		return 1;
	
	return 0;
}

funcCallbackOnXmlProperty CallbackOnXmlProperty ;
int CallbackOnXmlProperty( char *xpath , int xpath_len , int xpath_size , char *propname , int propname_len , char *propvalue , int propvalue_len , void *p )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *)p ;
	
	struct DoneQueueUnit	*pdqu = NULL ;
	
	int			nret = 0 ;
	
	if( propname_len == 4 && STRNICMP( propname , == , "href" , propname_len ) )
	{
		if( penv->max_recursive_depth > 1 && penv->pdqu->recursive_depth >= penv->max_recursive_depth )
			return 0;
		
		if( ! CheckHttpProtocol( propvalue ) )
			return 0;
		
		if( CheckFileExtname( penv , propvalue , propvalue_len ) )
		{
			char		url[ 4096 + 1 ] ;
			long		url_len ;
			
			memset( url , 0x00 , sizeof(url) );
			strcpy( url , penv->url );
			nret = FormatNewUrl( penv , propvalue , propvalue_len , url ) ;
			if( nret )
				return 0;
			url_len = strlen(url) ;
			
			InfoLog( __FILE__ , __LINE__ , ".a.href[%.*s]" , propvalue_len , propvalue );
			
			nret = GetHashItemPtr( & (penv->done_queue) , url , NULL , NULL ) ;
			if( nret == HASH_RETCODE_ERROR_KEY_NOT_EXIST )
			{
				nret = AddQueueBlock( penv->request_queue , url , url_len + 1 , NULL ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] failed[%d]" , url , nret );
					return -1;
				}
				else
				{
					DebugLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] ok" , url );
				}
				
				pdqu = AllocDoneQueueUnit( url , NULL , NULL , 0 , penv->pdqu->recursive_depth + 1 ) ;
				if( pdqu == NULL )
				{
					ErrorLog( __FILE__ , __LINE__ , "AllocDoneQueueUnit failed errno[%d]" , errno );
					return SIMSPIDER_ERROR_ALLOC;
				}
				
				nret = PutHashItem( & (penv->done_queue) , url , (void*) pdqu , sizeof(struct DoneQueueUnit) , & FreeDoneQueueUnit , HASH_PUTMODE_ADD ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "PutHashItem failed[%d] errno[%d]" , nret , errno );
					return -1;
				}
			}
			else if( nret < 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr failed[%d] errno[%d]" , nret , errno );
				return -1;
			}
		}
	}
	
	return 0;
}

funcParserHtmlNodeProc ParserHtmlNodeProc ;
int ParserHtmlNodeProc( int type , char *xpath , int xpath_len , int xpath_size , char *node , int node_len , char *properties , int properties_len , char *content , int content_len , void *p )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *)p ;
	
	int			nret = 0 ;
	
	if( penv->pfuncParserHtmlNodeProc )
	{
		nret = penv->pfuncParserHtmlNodeProc( type , xpath , xpath_len , xpath_size , node , node_len , properties , properties_len , content , content_len , p ) ;
		if( nret == SIMSPIDER_INFO_THEN_DO_IT_FOR_DEFAULT )
		{
		}
		else if( nret )
		{
			return nret;
		}
		else
		{
			return 0;
		}
	}
	
	if( type & FASTERXML_NODE_BRANCH )
	{
		if( type & FASTERXML_NODE_ENTER )
		{
			DebugLog( __FILE__ , __LINE__ , "ENTER-BRANCH[%.*s]" , xpath_len , xpath );
		}
		else if( type & FASTERXML_NODE_LEAVE )
		{
			DebugLog( __FILE__ , __LINE__ , "LEAVE-BRANCH[%.*s]" , xpath_len , xpath );
		}
	}
	
	if( type & FASTERXML_NODE_LEAF )
	{
		DebugLog( __FILE__ , __LINE__ , "LEAF        [%.*s] - [%.*s]" , xpath_len , xpath , content_len , content );
		
		if( node_len == 1 && STRNICMP( node , == , "a" , node_len ) )
		{
			nret = TravelXmlPropertiesBuffer( properties , properties_len , xpath , xpath_len , xpath_size , & CallbackOnXmlProperty , p );
			if( nret )
			{
				return nret;
			}
		}
	}
	
	return 0;
}

static size_t CurlResponseHeaderProc( char *buffer , size_t size , size_t nmemb , void *p )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *)p ;
	int			nret = 0 ;
	
	if( size*nmemb > penv->header.bufsize-1 - penv->header.len )
	{
		nret = ReallocHeaderBuffer( penv , penv->header.len + size*nmemb + 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "ReallocBodyBuffer failed[%d] errno[%d]" , nret , errno );
			return -1;
		}
	}
	memcpy( penv->header.base + penv->header.len , buffer , size*nmemb );
	penv->header.len += size*nmemb ;
	return size*nmemb;
}

static size_t CurlResponseBodyProc( char *buffer , size_t size , size_t nmemb , void *p )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *)p ;
	int			nret = 0 ;
	
	if( size*nmemb > penv->body.bufsize-1 - penv->body.len )
	{
		nret = ReallocBodyBuffer( penv , penv->body.len + size*nmemb + 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "ReallocBodyBuffer failed[%d] errno[%d]" , nret , errno );
			return -1;
		}
	}
	memcpy( penv->body.base + penv->body.len , buffer , size*nmemb );
	penv->body.len += size*nmemb ;
	return size*nmemb;
}

int SimSpiderGo( struct SimSpiderEnv *penv , char **entry_urls , char **custom_header , char **post_data )
{
	struct QueueBlock	*pqb = NULL ;
	int			len ;
	struct DoneQueueUnit	*pdqu = NULL ;
	
	CURL			*curl = NULL ;
	CURLcode		ret = CURLE_OK ;
	struct curl_slist	*chunk = NULL ;
	
	char			xpath[ 4096 + 1 ] ;
	int			xpath_bufsize ;
	
	int			nret = 0 ;
	
	if( penv == NULL )
		return 0;
	
	for( ; entry_urls && (*entry_urls) ; entry_urls++ )
	{
		nret = AddQueueBlock( penv->request_queue , *entry_urls , strlen(*entry_urls)+1 , NULL ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] failed[%d] errno[%d]" , *entry_urls , nret , errno );
			return SIMSPIDER_ERROR_ALLOC;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] ok" , *entry_urls );
		}
		
		pdqu = AllocDoneQueueUnit( *entry_urls , custom_header?(*custom_header):NULL , post_data?(*post_data):NULL , 0 , 1 ) ;
		if( pdqu == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "AllocDoneQueueUnit failed errno[%d]" , errno );
			return SIMSPIDER_ERROR_ALLOC;
		}
		nret = PutHashItem( & (penv->done_queue) , *entry_urls , (void*) pdqu , sizeof(struct DoneQueueUnit) , & FreeDoneQueueUnit , HASH_PUTMODE_SET ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "PutHashItem failed[%d]" , nret );
			return SIMSPIDER_ERROR_ALLOC;
		}
		
		if( custom_header )
			custom_header++;
		if( post_data )
			post_data++;
	}
	
	while(1)
	{
		nret = TravelQueueBlockByOrder( penv->request_queue , & pqb ) ;
		if( nret == MEMQUEUE_WARN_NO_BLOCK )
		{
			break;
		}
		else if( nret < 0 )
		{
			ErrorLog( __FILE__ , __LINE__ , "TravelQueueBlockByOrder failed[%d] errno[%d]" , nret , errno );
			return SIMSPIDER_ERROR_INTERNAL;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "TravelQueueBlockByOrder ok [%s]" , (char*)pqb + sizeof(struct QueueBlock) );
		}
		
		nret = GetHashItemPtr( & (penv->done_queue) , (char*)pqb + sizeof(struct QueueBlock) , (void**) & pdqu , NULL ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr2 failed[%d] errno[%d]" , nret , errno );
			return SIMSPIDER_ERROR_INTERNAL;
		}
		penv->pdqu = pdqu ;
		
		memset( penv->url , 0x00 , sizeof(penv->url) );
		len = snprintf( penv->url , sizeof(penv->url) , "%.*s" , (int)(pqb->block_size) , penv->pdqu->url ) ;
		if( len >= sizeof(penv->url)-1 )
		{
			ErrorLog( __FILE__ , __LINE__ , "url[%s] too long" , penv->url );
			pdqu->status = SIMSPIDER_ERROR_URL_TOOLONG ;
			goto _GOTO_CONTINUE;
		}
		
		CleanSimSpiderBuffer( penv );
		
		curl = curl_easy_init() ;
		if( curl == NULL )
		{
			ErrorLog( __FILE__ , __LINE__ , "curl_easy_init failed" );
			return SIMSPIDER_ERROR_INTERNAL;
		}
		
		curl_easy_setopt( curl , CURLOPT_URL , penv->url );
		curl_easy_setopt( curl , CURLOPT_VERBOSE , 0L );
		
		if( penv->pfuncResponseHeaderProc )
		{
			curl_easy_setopt( curl , CURLOPT_HEADER , 0 );
			curl_easy_setopt( curl , CURLOPT_HEADERFUNCTION , & CurlResponseHeaderProc );
			curl_easy_setopt( curl , CURLOPT_HEADERDATA , penv );
		}
		
		curl_easy_setopt( curl , CURLOPT_WRITEFUNCTION , & CurlResponseBodyProc );
		curl_easy_setopt( curl , CURLOPT_WRITEDATA , penv );
		
		if( STRNCMP( penv->url , == , "http:" , 5 ) )
		{
			InfoLog( __FILE__ , __LINE__ , "--- [%s] ------------------ HTTP" , penv->url );
		}
		else if( STRNCMP( penv->url , == , "https:" , 6 ) )
		{
			InfoLog( __FILE__ , __LINE__ , "--- [%s] ------------------ HTTPS" , penv->url );
			curl_easy_setopt( curl , CURLOPT_SSL_VERIFYPEER , 1L );
			curl_easy_setopt( curl , CURLOPT_CAINFO , penv->cert_pathfilename );
			curl_easy_setopt( curl , CURLOPT_SSL_VERIFYHOST , 1L );
		}
		
		if( pdqu->custom_header )
		{
			char		*custom_header = NULL ;
			char		*p = NULL ;
			
			DebugLog( __FILE__ , __LINE__ , "HTTPREQHEADER [%s]" , pdqu->custom_header );
			
			custom_header = strdup( pdqu->custom_header ) ;
			if( custom_header == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "strdup failed errno[%d]" , errno );
				return SIMSPIDER_ERROR_ALLOC;
			}
			
			p = strtok( custom_header , "\n" ) ;
			while( p )
			{
				chunk = curl_slist_append( chunk , p ) ;
				
				p = strtok( NULL , "\n" ) ;
			}
			
			ret = curl_easy_setopt( curl , CURLOPT_HTTPHEADER , chunk ) ;
			if( ret != CURLE_OK )
			{
				ErrorLog( __FILE__ , __LINE__ , "curl_easy_setopt CURLOPT_HTTPHEADER failed[%d]" , ret );
				pdqu->status = -ret ;
				goto _GOTO_CONTINUE;
			}
		}
		
		if( pdqu->post_data )
		{
			curl_easy_setopt( curl , CURLOPT_POSTFIELDS , pdqu->post_data );
		}
		
		ret = curl_easy_perform( curl ) ;
		if( ret != CURLE_OK )
		{
			ErrorLog( __FILE__ , __LINE__ , "curl_easy_perform failed[%d]" , ret );
			pdqu->status = -ret ;
			goto _GOTO_CONTINUE;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "curl_easy_perform ok" );
			if( penv->pfuncResponseHeaderProc )
			{
				DebugLog( __FILE__ , __LINE__ , "HTTPRSPHEADER [%.*s]" , penv->header.len , penv->header.base );
			}
			DebugLog( __FILE__ , __LINE__ , "HTTPRSPBODY   [%.*s]" , penv->body.len , penv->body.base );
		}
		
		if( penv->pfuncResponseHeaderProc )
		{
			nret = penv->pfuncResponseHeaderProc( penv->url , penv->header.base , & (penv->header.len) ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "pfuncResponseHeaderProc failed[%d]" , nret );
				pdqu->status = SIMSPIDER_ERROR_FUNCPROC ;
				goto _GOTO_CONTINUE;
			}
			DebugLog( __FILE__ , __LINE__ , "HTTPRSPHEADER2[%.*s]" , penv->header.len , penv->header.base );
		}
		
		if( penv->pfuncResponseBodyProc )
		{
			nret = penv->pfuncResponseBodyProc( pdqu->url , penv->body.base , & (penv->body.len) ) ;
			if( nret == SIMSPIDER_INFO_THEN_DO_IT_FOR_DEFAULT )
			{
				DebugLog( __FILE__ , __LINE__ , "HTTPRSPBODY2  [%.*s]" , penv->body.len , penv->body.base );
				goto _GOTO_THEN_PARSEHTML_FOR_DEFAULT;
			}
			else if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "pfuncResponseBodyProc failed[%d]" , nret );
				pdqu->status = SIMSPIDER_ERROR_FUNCPROC ;
				goto _GOTO_CONTINUE;
			}
			DebugLog( __FILE__ , __LINE__ , "HTTPRSPBODY2  [%.*s]" , penv->body.len , penv->body.base );
		}
		else
		{
_GOTO_THEN_PARSEHTML_FOR_DEFAULT :
			memset( xpath , 0x00 , sizeof(xpath) );
			xpath_bufsize = sizeof(xpath) ;
			nret = TravelXmlBuffer( penv->body.base , xpath , xpath_bufsize , & ParserHtmlNodeProc , penv ) ;
			if( nret && nret != FASTERXML_INFO_END_OF_BUFFER )
			{
				ErrorLog( __FILE__ , __LINE__ , "TravelXmlBuffer failed[%d]" , nret );
				pdqu->status = SIMSPIDER_ERROR_PARSEHTML ;
				goto _GOTO_CONTINUE;
			}
			else
			{
				DebugLog( __FILE__ , __LINE__ , "TravelXmlBuffer ok" );
			}
		}
		
		pdqu->status = 1 ;
		
_GOTO_CONTINUE :
		
		RemoveQueueBlock( penv->request_queue , pqb );
		
		curl_easy_cleanup( curl );
		
		if( pdqu->custom_header )
		{
			curl_slist_free_all( chunk );
		}
		
		if( penv->request_delay > 0 )
		{
			sleep( penv->request_delay );
		}
	}
	
	if( penv->pfuncTravelDoneQueueProc )
	{
		char		url[ 4096 + 1 ] ;
		
		memset( url , 0x00 , sizeof(url) );
		nret = TravelHashContainer( & (penv->done_queue) , url , sizeof(url) , penv->pfuncTravelDoneQueueProc , NULL );
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "TravelHashContainer failed[%d]" , ret );
			return SIMSPIDER_ERROR_INTERNAL;
		}
	}
	
	return 0;
}

