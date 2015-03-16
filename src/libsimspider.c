/*
 * simspider - Net Spider Engine
 * author	: calvin
 * email	: calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libsimspider.h"

#include "memque.h"
#include "HashX.h"
#include "LOGC.h"
#include "libsimspider.h"

char	__SIMSPIDER_VERSION_2_5_1[] = "2.5.1" ;
char	*__SIMSPIDER_VERSION = __SIMSPIDER_VERSION_2_5_1 ;

struct SimSpiderEnv
{
	CURLM				*curls ;
	
	char				valid_file_extname_set[ SIMSPIDER_VALID_FILE_EXTNAME_SET + 1 ] ;
	char				valid_html_file_extname_set[ SIMSPIDER_VALID_HTML_FILE_EXTNAME_SET + 1 ] ;
	int				allow_empty_file_extname ;
	char				cert_pathfilename[ SIMSPIDER_MAXLEN_FILENAME + 1 ] ;
	int				allow_runoutof_website ;
	long				max_recursive_depth ;
	long				request_delay ;
	int				concurrent_count_automode ;
	long				max_concurrent_count ;
	long				adjust_concurrent_count ;
	
	funcRequestHeaderProc		*pfuncRequestHeaderProc ;
	funcRequestBodyProc		*pfuncRequestBodyProc ;
	funcResponseHeaderProc		*pfuncResponseHeaderProc ;
	funcResponseBodyProc		*pfuncResponseBodyProc ;
	
	void				*request_queue_env ;
	funcPushRequestQueueUnitProc	*pfuncPushRequestQueueUnitProc ;
	funcTravelRequestQueueUnitProc	*pfuncTravelRequestQueueUnitProc ;
	funcPopupRequestQueueUnitProc	*pfuncPopupRequestQueueUnitProc ;
	
	void				*done_queue_env ;
	funcQueryDoneQueueUnitProc	*pfuncQueryDoneQueueUnitProc ;
	funcAddDoneQueueUnitProc	*pfuncAddDoneQueueUnitProc ;
	funcUpdateDoneQueueUnitProc	*pfuncUpdateDoneQueueUnitProc ;
	
	int				html_linker_parser_enable ;
	
	funcTravelDoneQueueProc		*pfuncTravelDoneQueueProc ;
	
	struct MemoryQueue		*request_queue ;
	struct HashContainer		done_queue ;
	
	void				*public_data ;
	
	unsigned long			move_unsuccessful_done_queue ;
} ;

struct DoneQueueUnit
{
	char			referer_url [ SIMSPIDER_MAXLEN_URL + 1 ] ;
	char			url [ SIMSPIDER_MAXLEN_URL + 1 ] ;
	char			*post_url ;
	int			status ;
	long			recursive_depth ;
	
	struct SimSpiderEnv	*penv ;
	CURL			*curl ;
	
	struct curl_slist	*free_curlheadlist_later ;
	struct curl_slist	*free_curllist1_later ;
	struct curl_slist	*free_curllist2_later ;
	struct curl_slist	*free_curllist3_later ;
	
	struct SimSpiderBuf	header ;
	struct SimSpiderBuf	body ;
	
	char			*private_data ;
} ;

static void _FreeDoneQueueUnit( struct DoneQueueUnit *pdqu )
{
	if( pdqu->header.base )
	{
		free( pdqu->header.base );
		pdqu->header.base = NULL ;
		pdqu->header.bufsize = 0 ;
		pdqu->header.len = 0 ;
	}
	if( pdqu->body.base )
	{
		free( pdqu->body.base );
		pdqu->body.base = NULL ;
		pdqu->body.bufsize = 0 ;
		pdqu->body.len = 0 ;
	}
	
	if( pdqu->free_curlheadlist_later )
	{
		curl_slist_free_all( pdqu->free_curlheadlist_later );
		pdqu->free_curlheadlist_later = NULL ;
	}
	if( pdqu->free_curllist1_later )
	{
		curl_slist_free_all( pdqu->free_curllist1_later );
		pdqu->free_curllist1_later = NULL ;
	}
	if( pdqu->free_curllist2_later )
	{
		curl_slist_free_all( pdqu->free_curllist2_later );
		pdqu->free_curllist2_later = NULL ;
	}
	if( pdqu->free_curllist3_later )
	{
		curl_slist_free_all( pdqu->free_curllist3_later );
		pdqu->free_curllist3_later = NULL ;
	}
	
	return;
}

static BOOL FreeDoneQueueUnit( void *pv )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)pv ;
	
	if( pdqu )
	{
		_FreeDoneQueueUnit( pdqu );
		
		free( pdqu );
	}
	
	return TRUE;
}

static struct DoneQueueUnit *AllocDoneQueueUnit( char *referer_url , char *url , int status , long recursive_depth )
{
	struct DoneQueueUnit	*pdqu = NULL ;
	long			header_bufsize = 4096 + 1 ;
	long			body_bufsize = 4096 + 1 ;
	
	pdqu = (struct DoneQueueUnit *)malloc( sizeof(struct DoneQueueUnit) ) ;
	if( pdqu == NULL )
		return NULL;
	memset( pdqu , 0x00 , sizeof(struct DoneQueueUnit) );
	
	if( referer_url )
		strncpy( pdqu->referer_url , referer_url , sizeof(pdqu->referer_url)-1 ) ;
	strncpy( pdqu->url , url , sizeof(pdqu->url)-1 ) ;
	
	pdqu->status = status ;
	pdqu->recursive_depth = recursive_depth ;
	
	pdqu->header.base = (char*)malloc( header_bufsize ) ;
	if( pdqu->header.base == NULL )
		return NULL;
	memset( pdqu->header.base , 0x00 , header_bufsize );
	pdqu->header.bufsize = header_bufsize ;
	pdqu->header.len = 0 ;
	
	pdqu->body.base = (char*)malloc( body_bufsize ) ;
	if( pdqu->body.base == NULL )
		return NULL;
	memset( pdqu->body.base , 0x00 , body_bufsize );
	pdqu->body.bufsize = body_bufsize ;
	pdqu->body.len = 0 ;
	
	return pdqu;
}

char *GetDoneQueueUnitRefererUrl( struct DoneQueueUnit *pdqu )
{
	return pdqu->referer_url;
}

char *GetDoneQueueUnitUrl( struct DoneQueueUnit *pdqu )
{
	return pdqu->url;
}

int GetDoneQueueUnitStatus( struct DoneQueueUnit *pdqu )
{
	return pdqu->status;
}

long GetDoneQueueUnitRecursiveDepth( struct DoneQueueUnit *pdqu )
{
	return pdqu->recursive_depth;
}

CURL *GetDoneQueueUnitCurl( struct DoneQueueUnit *pdqu )
{
	return pdqu->curl;
}

struct SimSpiderBuf *GetDoneQueueUnitHeaderBuffer( struct DoneQueueUnit *pdqu )
{
	return & (pdqu->header);
}

struct SimSpiderBuf *GetDoneQueueUnitBodyBuffer( struct DoneQueueUnit *pdqu )
{
	return & (pdqu->body);
}

void SetDoneQueueUnitPrivateDataPtr( struct DoneQueueUnit *pdqu , char *private_data )
{
	pdqu->private_data = private_data ;
	return;
}

char *GetDoneQueueUnitPrivateDataPtr( struct DoneQueueUnit *pdqu )
{
	return pdqu->private_data;
}

funcTravelDoneQueueProc TravelDoneQueueForMovingProc ;
void TravelDoneQueueForMovingProc( unsigned char *key , void *value , long value_len , void *pv )
{
	struct SimSpiderEnv	*penv = (struct SimSpiderEnv *) pv ;
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *) value ;
	int			nret = 0 ;
	
	if( pdqu->status == 200 )
		return;
	
	if( penv->pfuncPushRequestQueueUnitProc )
	{
		nret = penv->pfuncPushRequestQueueUnitProc( penv->request_queue_env , pdqu->url ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncPushRequestQueueUnitProc failed[%d]" , nret );
			return;
		}
	}
	else
	{
		nret = AddQueueBlock( penv->request_queue , pdqu->url , strlen(pdqu->url) + 1 , NULL ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] failed[%d]" , pdqu->url , nret );
			return;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "AddQueueBlock[%s][%ld] ok" , pdqu->url , pdqu->recursive_depth );
		}
	}
	
	penv->move_unsuccessful_done_queue++;
	
	return;
}

unsigned long MoveUnsuccessfulDoneQueueUnitsToRequestQueue( struct SimSpiderEnv *penv )
{
	char		url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
	int		nret = 0 ;
	
	memset( url , 0x00 , sizeof(url) );
	penv->move_unsuccessful_done_queue = 0 ;
	nret = TravelHashContainer( & (penv->done_queue) , (unsigned char *) url , sizeof(url) , & TravelDoneQueueForMovingProc , (void*)penv );
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "TravelHashContainer failed[%d]" , nret );
		return SIMSPIDER_ERROR_INTERNAL;
	}
	
	return penv->move_unsuccessful_done_queue;
}

void ResetDoneQueue( struct SimSpiderEnv *penv )
{
	DeleteAllHashItem( & (penv->done_queue) );
	return;
}

void SetSimSpiderPublicDataPtr( struct SimSpiderEnv *penv , void *public_data )
{
	penv->public_data = public_data ;
	return;
}

void *GetSimSpiderPublicDataPtr( struct DoneQueueUnit *pdqu )
{
	return pdqu->penv->public_data;
}

static struct SimSpiderEnv *AllocSimSpiderEnv()
{
	struct SimSpiderEnv	*penv = NULL ;
	
	penv = (struct SimSpiderEnv *)malloc( sizeof(struct SimSpiderEnv) ) ;
	if( penv == NULL )
		return NULL;
	memset( penv , 0x00 , sizeof(struct SimSpiderEnv) );
	
	return penv;
}

int ReallocHeaderBuffer( struct DoneQueueUnit *pdqu , long new_bufsize )
{
	char	*new_base = NULL ;
	
	if( new_bufsize <= pdqu->header.bufsize )
		return 0;
	
	new_base = (char*)realloc( pdqu->header.base , new_bufsize ) ;
	if( new_base == NULL )
		return SIMSPIDER_ERROR_ALLOC;
	memset( new_base + pdqu->header.len , 0x00 , new_bufsize - pdqu->header.len );
	
	pdqu->header.base = new_base ;
	pdqu->header.bufsize = new_bufsize ;
	
	return 0;
}

int ReallocBodyBuffer( struct DoneQueueUnit *pdqu , long new_bufsize )
{
	char	*new_base = NULL ;
	
	if( new_bufsize <= pdqu->body.bufsize )
		return 0;
	
	new_base = (char*)realloc( pdqu->body.base , new_bufsize ) ;
	if( new_base == NULL )
		return SIMSPIDER_ERROR_ALLOC;
	memset( new_base + pdqu->body.len , 0x00 , new_bufsize - pdqu->body.len );
	
	pdqu->body.base = new_base ;
	pdqu->body.bufsize = new_bufsize ;
	
	return 0;
}

int CleanSimSpiderBuffer( struct DoneQueueUnit *pdqu )
{
	memset( pdqu->header.base , 0x00 , pdqu->header.bufsize );
	pdqu->header.len = 0 ;
	memset( pdqu->body.base , 0x00 , pdqu->body.bufsize );
	pdqu->body.len = 0 ;
	return 0;
}

struct curl_slist *GetCurlHeadListPtr( struct DoneQueueUnit *pdqu )
{
	return pdqu->free_curlheadlist_later;
}

void FreeCurlHeadList1Later( struct DoneQueueUnit *pdqu , struct curl_slist *curlheadlist )
{
	pdqu->free_curlheadlist_later = curlheadlist ;
}

void FreeCurlList1Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist1 )
{
	pdqu->free_curllist1_later = curllist1 ;
}

void FreeCurlList2Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist2 )
{
	pdqu->free_curllist2_later = curllist2 ;
}

void FreeCurlList3Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist3 )
{
	pdqu->free_curllist3_later = curllist3 ;
}

static void FreeSimSpiderEnv( struct SimSpiderEnv *penv )
{
	free( penv );
	return;
}

int InitSimSpiderEnv( struct SimSpiderEnv **ppenv , char *log_file_format , ... )
{
	int		nret = 0 ;
	
	srand( (unsigned int)time( NULL ) );
	
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
			return SIMSPIDER_ERROR_GETENV;
	}
	else
	{
		SetLogLevel( LOGLEVEL_DEBUG );
	}
	
	(*ppenv) = AllocSimSpiderEnv() ;
	if( (*ppenv) == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "AllocSimSpiderEnv failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_ALLOC;
	}
	
	nret = CreateMemoryQueue( & ((*ppenv)->request_queue) , SIMSPIDER_DEFAULT_REQUESTQUEUE_SIZE , -1 , -1 ) ;
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
	
	curl_global_init( CURL_GLOBAL_DEFAULT );
	(*ppenv)->curls = curl_multi_init() ;
	
	SetValidFileExtnameSet( (*ppenv) , SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION );
	AllowEmptyFileExtname( (*ppenv) , 1 );
	AllowRunOutofWebsite( (*ppenv) , 0 );
	SetMaxConcurrentCount( (*ppenv) , 1 );
	EnableHtmlLinkerParser( (*ppenv) , 1 );
	
	return 0;
}

void CleanSimSpiderEnv( struct SimSpiderEnv **ppenv )
{
	DestroyMemoryQueue( & ((*ppenv)->request_queue) );
	CleanHashContainer( & ((*ppenv)->done_queue) );
	
	curl_multi_cleanup( (*ppenv)->curls );
	curl_global_cleanup();
	
	FreeSimSpiderEnv( (*ppenv) );
	
	return;
}

void ResetSimSpiderEnv( struct SimSpiderEnv *penv )
{
	CleanMemoryQueue( penv->request_queue );
	DeleteAllHashItem( & (penv->done_queue) );
	
	return;
}

int ResizeRequestQueue( struct SimSpiderEnv *penv , long size )
{
	int	nret = 0 ;
	
	nret = DestroyMemoryQueue( & (penv->request_queue) ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "DestroyMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_MEMQUEUE;
	}
	
	nret = CreateMemoryQueue( & (penv->request_queue) , size , -1 , -1 ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "CreateMemoryQueue failed[%d] errno[%d]" , nret , errno );
		return SIMSPIDER_ERROR_LIB_MEMQUEUE;
	}
	
	return 0;
}

void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set )
{
	memset( penv->valid_file_extname_set , 0x00 , sizeof(penv->valid_file_extname_set) );
	SNPRINTF( penv->valid_file_extname_set , sizeof(penv->valid_file_extname_set)-1 , " %s " , valid_file_extname_set );
	return;
}

void SetValidHtmlFileExtnameSet( struct SimSpiderEnv *penv , char *valid_html_file_extname_set )
{
	memset( penv->valid_html_file_extname_set , 0x00 , sizeof(penv->valid_html_file_extname_set) );
	SNPRINTF( penv->valid_html_file_extname_set , sizeof(penv->valid_html_file_extname_set)-1 , " %s " , valid_html_file_extname_set );
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
	VSNPRINTF( penv->cert_pathfilename , sizeof(penv->cert_pathfilename)-1 , cert_pathfilename_format , valist );
	va_end( valist );
	
	return;
}

void SetRequestDelay( struct SimSpiderEnv *penv , long seconds )
{
	penv->request_delay = seconds ;
	return;
}

void SetMaxConcurrentCount( struct SimSpiderEnv *penv , long max_concurrent_count )
{
	if( max_concurrent_count == SIMSPIDER_CONCURRENTCOUNT_AUTO )
	{
		penv->concurrent_count_automode = 1 ;
		penv->max_concurrent_count = 1 ;
	}
	else
	{
		penv->concurrent_count_automode = 0 ;
		penv->max_concurrent_count = max_concurrent_count ;
	}
	
#if CURLMOPT_MAXCONNECTS
	curl_multi_setopt( penv->curls , CURLMOPT_MAXCONNECTS , penv->max_concurrent_count );
#endif
	
	return;
}

void SetRequestHeaderProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestHeaderProc )
{
	penv->pfuncRequestHeaderProc = pfuncRequestHeaderProc ;
	return;
}

void SetRequestBodyProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestBodyProc )
{
	penv->pfuncRequestBodyProc = pfuncRequestBodyProc ;
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

void EnableHtmlLinkerParser( struct SimSpiderEnv *penv , int html_linker_parser_enable )
{
	penv->html_linker_parser_enable = html_linker_parser_enable ;
}

void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc )
{
	penv->pfuncTravelDoneQueueProc = pfuncTravelDoneQueueProc ;
	return;
}

void SetRequestQueueEnv( struct SimSpiderEnv *penv , void *request_queue_env )
{
	penv->request_queue_env = request_queue_env ;
}

void SetPushRequestQueueUnitProc( struct SimSpiderEnv *penv , funcPushRequestQueueUnitProc *pfuncPushRequestQueueUnitProc )
{
	penv->pfuncPushRequestQueueUnitProc = pfuncPushRequestQueueUnitProc ;
	return;
}

void SetTravelRequestQueueUnitProc( struct SimSpiderEnv *penv , funcTravelRequestQueueUnitProc *pfuncTravelRequestQueueUnitProc )
{
	penv->pfuncTravelRequestQueueUnitProc = pfuncTravelRequestQueueUnitProc ;
	return;
}

void SetPopupRequestQueueUnitProc( struct SimSpiderEnv *penv , funcPopupRequestQueueUnitProc *pfuncPopupRequestQueueUnitProc )
{
	penv->pfuncPopupRequestQueueUnitProc = pfuncPopupRequestQueueUnitProc ;
	return;
}

void SetDoneQueueEnv( struct SimSpiderEnv *penv , void *done_queue_env )
{
	penv->done_queue_env = done_queue_env ;
}

void SetQueryDoneQueueUnitProc( struct SimSpiderEnv *penv , funcQueryDoneQueueUnitProc *pfuncQueryDoneQueueUnitProc )
{
	penv->pfuncQueryDoneQueueUnitProc = pfuncQueryDoneQueueUnitProc ;
	return;
}

void SetAddDoneQueueUnitProc( struct SimSpiderEnv *penv , funcAddDoneQueueUnitProc *pfuncAddDoneQueueUnitProc )
{
	penv->pfuncAddDoneQueueUnitProc = pfuncAddDoneQueueUnitProc ;
	return;
}

void SetUpdateDoneQueueUnitProc( struct SimSpiderEnv *penv , funcUpdateDoneQueueUnitProc *pfuncUpdateDoneQueueUnitProc )
{
	penv->pfuncUpdateDoneQueueUnitProc = pfuncUpdateDoneQueueUnitProc ;
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

static char *strnchr( char *str , int str_len , int c )
{
	char		*ptr = NULL ;
	for( ptr = str ; ptr < str + str_len ; ptr++ )
	{
		if( (*ptr) == (char)c )
			return ptr;
	}
	
	return NULL;
}

static char *strnstr( char *str , int str_len , char *find )
{
	char	*match = NULL ;
	int	offset ;
	
	for( offset = 0 ; (*str) && offset < str_len ; str++ , offset++ )
	{
		if( match == NULL )
		{
			if( (*str) == (*find) )
			{
				match = find + 1 ;
			}
		}
		else
		{
			if( (*str) == (*match) )
			{
				match++;
				if( (*match) == '\0' )
					return str - ( match - find ) + 1 ;
			}
			else
			{
				str -= ( match - find ) ;
				offset -= ( match - find ) ;
				match = NULL ;
			}
		}
	}
	
	return NULL;
}

#if ( defined _WIN32 )
int my_strncmp(const char* dst,const char* src,int len)
{
    int ch1,ch2;
    len--;
    do
    {
      if ( ((ch1 = (unsigned char)(*(dst++))) >= 'A') &&(ch1 <= 'Z') )
        ch1 += 0x20;
    //printf("ch1=%c\n",ch1);
      if ( ((ch2 = (unsigned char)(*(src++))) >= 'A') &&(ch2 <= 'Z') )
        ch2 += 0x20;
    //printf("ch2=%c\n",ch2);
    }while(ch1&&ch2&&(ch1 == ch2)&&len--);
   return(ch1 - ch2);
}
char* strcasestr(const char* s1, const char* s2)
{
    int len2 = strlen(s2); /* 获得待查找串的长度*/
    int tries; /* maximum number of comparisons */
    int nomatch = 1; /* set to 0 if match is found */
   
    tries = strlen(s1) + 1 - len2; /*此处说明最多只用比较这么多次，*/
    if (tries > 0)
        while (( nomatch = my_strncmp(s1, s2, len2)) && tries--)
            s1++;
    if (nomatch)
        return NULL;
    else
        return (char *) s1; /* cast const away */
}
#endif

static int FormatNewUrl( struct SimSpiderEnv *penv , char *propvalue , int propvalue_len , char *url )
{
	int		url_len ;
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
			return 1;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 1;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 1;
		if( STRNCMP( propvalue , != , url , ptr+1-url ) && penv->allow_runoutof_website == 0 )
			return 1;
		if( propvalue_len > SIMSPIDER_MAXLEN_URL )
			return SIMSPIDER_ERROR_URL_TOOLONG;
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
			return 1;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 1;
		ptr = strchr( ptr+1 , '/' ) ;
		if( ptr == NULL )
			return 1;
		if( ptr-url+propvalue_len > SIMSPIDER_MAXLEN_URL )
			return SIMSPIDER_ERROR_URL_TOOLONG;
		SNPRINTF( ptr+1 , SIMSPIDER_MAXLEN_URL-(ptr-url) , "%.*s" , propvalue_len , propvalue );
	}
	else
	{
		url_len = strlen(url) ;
		
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
			return SIMSPIDER_ERROR_URL_INVALID;
		}
		else if( (*ptr) == '/' )
		{
			SNPRINTF( url + url_len , SIMSPIDER_MAXLEN_URL - url_len - 1 , "%.*s" , propvalue_len , propvalue );
		}
		else
		{
			ptr2 = strrnchr( url , ptr - url , '/' ) ;
			if( (*ptr) == '.' && ptr2 && url + 2 <= ptr2 && MEMCMP( ptr2 - 2 , == , ":/" , 2 ) )
			{
				if( url_len+propvalue_len > SIMSPIDER_MAXLEN_URL )
					return SIMSPIDER_ERROR_URL_TOOLONG;
				SNPRINTF( url + url_len , SIMSPIDER_MAXLEN_URL - url_len - 1 , "/%.*s" , propvalue_len , propvalue );
			}
			else
			{
				ptr = strrnchr( url , ptr - url , '/' ) ;
				if( ptr == NULL )
					return 1;
				if( ptr-url+propvalue_len > SIMSPIDER_MAXLEN_URL )
					return SIMSPIDER_ERROR_URL_TOOLONG;
				SNPRINTF( ptr+1 , SIMSPIDER_MAXLEN_URL-(ptr-url) , "%.*s" , propvalue_len , propvalue );
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
	
	ptr = strstr( url , "://" ) ;
	if( ptr )
		ptr += 3 ;
	else
		ptr = url ;
	while(1)
	{
		ptr = strstr( ptr , "//" ) ;
		if( ptr == NULL )
			break;
		memmove( ptr , ptr+1 , strlen(ptr+1)+1 );
		ptr++;
	}
	
	ptr = strchr( url , '#' ) ;
	if( ptr )
		*(ptr) = '\0' ;
	
	return 0;
}

static int CheckHttpProtocol( char *propvalue , int propvalue_len )
{
	char	*ptr = NULL ;
	int	len ;
	
	ptr = strnstr( propvalue , propvalue_len , "://" ) ;
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

static int CheckFileExtname( struct SimSpiderEnv *penv , char *valid_extname_set , char *propvalue , int propvalue_len )
{
	char		*base = NULL ;
	int		len ;
	char		*end = NULL , *begin = NULL ;
	char		file_extname[ 64 + 1 ] ;
	
	if( *(propvalue+propvalue_len-1) == '/' )
		return 1;
	
	if( propvalue_len == 1 && propvalue[0] == '#' )
		return 0;
	if( STRNCMP( propvalue , == , "javascript:" , 11 ) )
		return 0;
	if( STRNCMP( propvalue , == , "mailto:" , 7 ) )
		return 0;
	
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
	if( STRNICMP( propvalue , == , "http" , 4 ) )
	{
		base = strnstr( propvalue , propvalue_len , "://" ) ;
		if( base == NULL )
			return 0;
		base += 3 ;
		base = strnchr( base , propvalue_len - ( base - propvalue ) , '/' ) ;
		if( base == NULL )
		{
			if( penv->allow_empty_file_extname == 0 )
				return 0;
			else
				return 1;
		}
		len = propvalue_len - ( base - propvalue ) ;
	}
	else
	{
		base = propvalue ;
		len = propvalue_len ;
	}
	
	end = strrnchr( base , len , '?' ) ;
	if( end == NULL )
	{
		end = base + len ;
	}
	
	begin = strprnbrk( base , end - base , "/." ) ;
	if( begin )
	{
		if( (*begin) == '/' )
		{
			if( penv->allow_empty_file_extname == 0 )
				return 0;
			else
				return 1;
		}
		begin++;
	}
	else
	{
		if( penv->allow_empty_file_extname == 0 )
			return 0;
		else
			return 1;
	}
	
	memset( file_extname , 0x00 , sizeof(file_extname) );
	SNPRINTF( file_extname , sizeof(file_extname) , " %.*s " , (int)(end-begin) , begin );
	if( strstr( valid_extname_set , file_extname ) )
		return 1;
	
	return 0;
}

int AppendRequestQueue( struct SimSpiderEnv *penv , char *referer_url , char *url , long depth )
{
	char			format_url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
	struct DoneQueueUnit	*pdqu = NULL ;
	
	int			nret = 0 ;
	
	memset( format_url , 0x00 , sizeof(format_url) );
	if( STRNICMP( url , != , "http:" , 4 ) )
		SNPRINTF( format_url , sizeof(format_url) , "http://%s/" , url );
	else
		SNPRINTF( format_url , sizeof(format_url) , "%s" , url );
	
	pdqu = AllocDoneQueueUnit( referer_url , format_url , 0 , depth ) ;
	if( pdqu == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "AllocDoneQueueUnit failed errno[%d]" , errno );
		return SIMSPIDER_ERROR_ALLOC;
	}
	pdqu->penv = penv ;
	
	if( penv->pfuncPushRequestQueueUnitProc )
	{
		nret = penv->pfuncPushRequestQueueUnitProc( penv->request_queue_env , format_url ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncPushRequestQueueUnitProc failed[%d]" , nret );
			return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
		}
	}
	else
	{
		nret = AddQueueBlock( penv->request_queue , format_url , strlen(format_url) + 1 , NULL ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] failed[%d]" , format_url , nret );
			return SIMSPIDER_ERROR_REQUEST_QUEUE_OVERFLOW;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "AddQueueBlock[%s][%ld] ok" , format_url , depth );
		}
	}
	
	if( penv->pfuncAddDoneQueueUnitProc )
	{
		nret = penv->pfuncAddDoneQueueUnitProc( penv->done_queue_env , pdqu ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncSetAddDoneQueueUnitProc failed[%d]" , nret );
			return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
		}
	}
	else
	{
		nret = PutHashItem( & (penv->done_queue) , (unsigned char *) format_url , (void*) pdqu , sizeof(struct DoneQueueUnit) , & FreeDoneQueueUnit , HASH_PUTMODE_ADD ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "PutHashItem failed[%d] errno[%d]" , nret , errno );
			FreeDoneQueueUnit( pdqu );
			return SIMSPIDER_ERROR_LIB_HASHX;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "PutHashItem[%s] ok" , format_url );
		}
	}
	
	return 0;
}

#define FIND_HREF		" href="

char *strcasestr(const char *haystack, const char *needle);

static int HtmlLinkParser( char *buffer , struct DoneQueueUnit *pdqu )
{
	char		*propvalue = NULL ;
	int		propvalue_len ;
	char		begin_char ;
	char		*ptr = NULL ;
	int		nret = 0 ;
	
	propvalue = buffer ;
	while(1)
	{
		propvalue = strcasestr( propvalue , FIND_HREF ) ;
		if( propvalue == NULL )
			break;
		propvalue += sizeof(FIND_HREF)-1 ;
		
		if( strchr( "\"'" , *(propvalue) ) )
		{
			begin_char = *(propvalue) ;
			propvalue++;
		}
		else
		{
			begin_char = '\0' ;
		}
		
		if( begin_char )
		{
			for( ptr = propvalue , propvalue_len = 0 ; *(ptr) ; ptr++ )
			{
				if( *(ptr) == begin_char )
					break;
				propvalue_len++;
			}
		}
		else
		{
			for( ptr = propvalue , propvalue_len = 0 ; *(ptr) ; ptr++ )
			{
				if( *(ptr) == ' ' || *(ptr) == '/' || *(ptr) == '>' )
					break;
				propvalue_len++;
			}
		}
		
		if( pdqu->penv->max_recursive_depth > 1 && pdqu->recursive_depth >= pdqu->penv->max_recursive_depth )
			return 0;
		
		nret = CheckHttpProtocol( propvalue , propvalue_len ) ;
		if( nret == 0 )
			continue;
		
		if( pdqu->penv->valid_file_extname_set[0] == '\0' )
		{
			nret = 1 ;
		}
		else
		{
			nret = CheckFileExtname( pdqu->penv , pdqu->penv->valid_file_extname_set , propvalue , propvalue_len ) ;
		}
		if( nret == 1 )
		{
			char		url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
			long		url_len ;
			
			memset( url , 0x00 , sizeof(url) );
			strcpy( url , pdqu->url );
			nret = FormatNewUrl( pdqu->penv , propvalue , propvalue_len , url ) ;
			if( nret > 0 )
			{
				InfoLog( __FILE__ , __LINE__ , "FormatNewUrl[%.*s][%s] return[%d]" , propvalue_len , propvalue , url , nret );
				continue;
			}
			else if( nret < 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "FormatNewUrl[%.*s][%s] failed[%d]" , propvalue_len , propvalue , url , nret );
				continue;
			}
			url_len = strlen(url) ;
			
			InfoLog( __FILE__ , __LINE__ , ".a.href[%.*s] URL[%s]" , propvalue_len , propvalue , url );
			
			if( pdqu->penv->pfuncQueryDoneQueueUnitProc )
			{
				nret = pdqu->penv->pfuncQueryDoneQueueUnitProc( pdqu->penv->done_queue_env , pdqu ) ;
				if( nret < 0 )
				{
					ErrorLog( __FILE__ , __LINE__ , "pfuncQueryDoneQueueUnitProc failed[%d]" , nret );
					return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
				}
			}
			else
			{
				nret = GetHashItemPtr( & (pdqu->penv->done_queue) , (unsigned char *) url , NULL , NULL ) ;
				if( nret == HASH_RETCODE_ERROR_KEY_NOT_EXIST )
				{
					nret = 0 ;
				}
				else if( nret < 0 )
				{
					ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr[%s] failed[%d] errno[%d]" , url , nret , errno );
					return SIMSPIDER_ERROR_LIB_HASHX;
				}
				else
				{
					nret = 1 ;
				}
			}
			
			if( nret == 0 )
			{
				if( pdqu->penv->pfuncPushRequestQueueUnitProc )
				{
					nret = pdqu->penv->pfuncPushRequestQueueUnitProc( pdqu->penv->request_queue_env , url ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "pfuncPushRequestQueueUnitProc failed[%d]" , nret );
						return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
					}
				}
				else
				{
					nret = AppendRequestQueue( pdqu->penv , pdqu->url , url , pdqu->recursive_depth + 1 ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "AppendRequestQueue failed[%d]" , nret );
						return nret;
					}
				}
			}
		}
		
		propvalue += propvalue_len ;
	}
	
	return 0;
}

size_t CurlHeaderProc( char *buffer , size_t size , size_t nmemb , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	int			nret = 0 ;
	
	if( (long)(size*nmemb) > pdqu->header.bufsize-1 - pdqu->header.len )
	{
		nret = ReallocHeaderBuffer( pdqu , pdqu->header.len + size*nmemb + 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "ReallocBodyBuffer failed[%d] errno[%d]" , nret , errno );
			return nret;
		}
	}
	memcpy( pdqu->header.base + pdqu->header.len , buffer , size*nmemb );
	pdqu->header.len += size*nmemb ;
	return size*nmemb;
}

size_t CurlBodyProc( char *buffer , size_t size , size_t nmemb , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	int			nret = 0 ;
	
	if( (long)(size*nmemb) > pdqu->body.bufsize-1 - pdqu->body.len )
	{
		nret = ReallocBodyBuffer( pdqu , pdqu->body.len + size*nmemb + 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "ReallocBodyBuffer failed[%d] errno[%d]" , nret , errno );
			return nret;
		}
	}
	memcpy( pdqu->body.base + pdqu->body.len , buffer , size*nmemb );
	pdqu->body.len += size*nmemb ;
	return size*nmemb;
}

size_t CurlDebugProc( CURL *curl , curl_infotype type , char *buffer , size_t size , void *p )
{
	if( type == CURLINFO_HEADER_IN || type == CURLINFO_HEADER_OUT )
	{
		DebugLog( __FILE__ , __LINE__ , "[%.*s]" , size , buffer );
	}
	
	return 0;
}

static int FetchTasksFromRequestQueue( struct SimSpiderEnv *penv , int *p_still_running , CURL **pp_curl )
{
	char			url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
	struct QueueBlock	*pqb = NULL ;
	struct DoneQueueUnit	*pdqu = NULL ;	
	char			*post_data = NULL ;
	
	int			nret = 0 ;
	
	for( ; (*p_still_running) < penv->max_concurrent_count ; (*p_still_running)++ )
	{
		memset( url , 0x00 , sizeof(url) );
		if( penv->pfuncTravelRequestQueueUnitProc )
		{
			nret = penv->pfuncTravelRequestQueueUnitProc( penv->request_queue_env , url ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "pfuncTravelRequestQueueUnitProc failed[%d]" , nret );
				return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
			}
		}
		else
		{
			pqb = NULL ;
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
				strcpy( url , (char*)pqb + sizeof(struct QueueBlock) );
				DebugLog( __FILE__ , __LINE__ , "TravelQueueBlockByOrder[%s] ok" , url );
			}
		}
		
		if( penv->pfuncQueryDoneQueueUnitProc )
		{
			strcpy( pdqu->url , url );
			nret = penv->pfuncQueryDoneQueueUnitProc( penv->done_queue_env , pdqu ) ;
			if( nret < 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "pfuncQueryDoneQueueUnitProc failed[%d]" , nret );
				return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
			}
		}
		else
		{
			nret = GetHashItemPtr( & (penv->done_queue) , (unsigned char *) url , (void**) & pdqu , NULL ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr2[%s] failed[%d] errno[%d]" , url , nret , errno );
				return SIMSPIDER_ERROR_INTERNAL;
			}
			else
			{
				DebugLog( __FILE__ , __LINE__ , "GetHashItemPtr2[%s] ok" , url );
			}
		}
		
		CleanSimSpiderBuffer( pdqu );
		
		pdqu->penv = penv ;
		
		if( pp_curl && (*pp_curl) )
		{
			DebugLog( __FILE__ , __LINE__ , "reuse ok , curl[%p]" , (*pp_curl) );
			pdqu->curl = (*pp_curl) ;
			(*pp_curl) = NULL ;
		}
		else
		{
			pdqu->curl = curl_easy_init() ;
			if( pdqu->curl == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "curl_easy_init failed" );
				return SIMSPIDER_ERROR_INTERNAL;
			}
			else
			{
				DebugLog( __FILE__ , __LINE__ , "curl_easy_init ok , curl[%p]" , pdqu->curl );
			}
			
			if( STRNCMP( pdqu->url , == , "http:" , 5 ) )
			{
			}
			else if( STRNCMP( pdqu->url , == , "https:" , 6 ) )
			{
				curl_easy_setopt( pdqu->curl , CURLOPT_SSL_VERIFYPEER , 1L );
				curl_easy_setopt( pdqu->curl , CURLOPT_CAINFO , penv->cert_pathfilename );
				curl_easy_setopt( pdqu->curl , CURLOPT_SSL_VERIFYHOST , 1L );
			}
			
			curl_easy_setopt( pdqu->curl , CURLOPT_COOKIEFILE , "" );
			curl_easy_setopt( pdqu->curl , CURLOPT_TCP_NODELAY , 1L );
			curl_easy_setopt( pdqu->curl , CURLOPT_FOLLOWLOCATION , 1L );
			curl_easy_setopt( pdqu->curl , CURLOPT_VERBOSE , 1L );
			curl_easy_setopt( pdqu->curl , CURLOPT_DEBUGFUNCTION , & CurlDebugProc );
			curl_easy_setopt( pdqu->curl , CURLOPT_DEBUGDATA , NULL );
			if( penv->pfuncResponseHeaderProc )
			{
				curl_easy_setopt( pdqu->curl , CURLOPT_HEADER , 0 );
				curl_easy_setopt( pdqu->curl , CURLOPT_HEADERFUNCTION , & CurlHeaderProc );
			}
			curl_easy_setopt( pdqu->curl , CURLOPT_WRITEFUNCTION , & CurlBodyProc );
			curl_easy_setopt( pdqu->curl , CURLOPT_TIMEOUT , 60L );
			curl_easy_setopt( pdqu->curl , CURLOPT_CONNECTTIMEOUT , 60L );
		}
		
		if( GetDoneQueueUnitRefererUrl(pdqu)[0] )
		{
			char		buffer[ 9 + SIMSPIDER_MAXLEN_URL + 1 ] ;
			
			SNPRINTF( buffer , sizeof(buffer) , "Referer: %s" , GetDoneQueueUnitRefererUrl(pdqu) );
			pdqu->free_curlheadlist_later = curl_slist_append( pdqu->free_curlheadlist_later , buffer ) ;
			if( pdqu->free_curlheadlist_later == NULL )
			{
				ErrorLog( __FILE__ , __LINE__ , "curl_slist_append failed" );
				return SIMSPIDER_ERROR_ALLOC;
			}
		}
		
		if( penv->pfuncRequestHeaderProc )
		{
			nret = penv->pfuncRequestHeaderProc( pdqu ) ;
			if( nret )
			{
				pdqu->status = nret ;
				if( penv->pfuncPopupRequestQueueUnitProc )
				{
					nret = penv->pfuncPopupRequestQueueUnitProc( penv->request_queue_env , url ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "pfuncPopupRequestQueueUnitProc failed[%d]" , nret );
						return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
					}
				}
				else
				{
					RemoveQueueBlock( pdqu->penv->request_queue , pqb );
				}
				if( nret == SIMSPIDER_ERROR_FUNCPROC_INTERRUPT )
					return nret;
			}
		}
		
		curl_easy_setopt( pdqu->curl , CURLOPT_HTTPHEADER , pdqu->free_curlheadlist_later );
		
		post_data = strstr( pdqu->url , "??" ) ;
		if( post_data )
		{
			pdqu->post_url = strdup( pdqu->url ) ;
			if( pdqu->post_url == NULL )
				return SIMSPIDER_ERROR_ALLOC;
			pdqu->post_url[ post_data - pdqu->url ] = '\0' ;
			curl_easy_setopt( pdqu->curl , CURLOPT_URL , pdqu->post_url );
			curl_easy_setopt( pdqu->curl , CURLOPT_POSTFIELDS , post_data + 2 );
		}
		else
		{
			curl_easy_setopt( pdqu->curl , CURLOPT_URL , pdqu->url );
		}
		
		if( penv->pfuncResponseHeaderProc )
		{
			curl_easy_setopt( pdqu->curl , CURLOPT_HEADERDATA , pdqu );
		}
		
		InfoLog( __FILE__ , __LINE__ , "--- [%s] ------------------ HTTP" , pdqu->url );
		
		curl_easy_setopt( pdqu->curl , CURLOPT_WRITEDATA , pdqu );
		curl_easy_setopt( pdqu->curl , CURLOPT_PRIVATE , pdqu );
		
		if( penv->pfuncRequestBodyProc )
		{
			nret = penv->pfuncRequestBodyProc( pdqu ) ;
			if( nret )
			{
				pdqu->status = nret ;
				if( penv->pfuncPopupRequestQueueUnitProc )
				{
					nret = penv->pfuncPopupRequestQueueUnitProc( penv->request_queue_env , url ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "pfuncPopupRequestQueueUnitProc failed[%d]" , nret );
						return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
					}
				}
				else
				{
					RemoveQueueBlock( pdqu->penv->request_queue , pqb );
				}
				if( nret == SIMSPIDER_ERROR_FUNCPROC_INTERRUPT )
					return nret;
			}
		}
		
		curl_multi_add_handle( penv->curls , pdqu->curl );
		
		if( penv->pfuncPopupRequestQueueUnitProc )
		{
			nret = penv->pfuncPopupRequestQueueUnitProc( penv->request_queue_env , url ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "pfuncPopupRequestQueueUnitProc failed[%d]" , nret );
				return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
			}
		}
		else
		{
			RemoveQueueBlock( pdqu->penv->request_queue , pqb );
		}
	}
	
	return 0;
}

static int ProcessingTask( struct DoneQueueUnit *pdqu )
{
	int		nret = 0 ;
	
	if( pdqu->penv->pfuncResponseHeaderProc )
	{
		nret = pdqu->penv->pfuncResponseHeaderProc( pdqu ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncResponseHeaderProc failed[%d]" , nret );
			pdqu->status = nret ;
			if( nret == SIMSPIDER_ERROR_FUNCPROC_INTERRUPT )
				return nret;
		}
	}
	
	if(
		pdqu->penv->html_linker_parser_enable
		&&
		(
			pdqu->penv->max_recursive_depth == 0
			||
			(
				pdqu->penv->max_recursive_depth > 1 && pdqu->recursive_depth < pdqu->penv->max_recursive_depth
			)
		)
		&&
		CheckFileExtname( pdqu->penv , pdqu->penv->valid_html_file_extname_set , pdqu->url , strlen(pdqu->url) ) == 1
	)
	{
		nret = HtmlLinkParser( pdqu->body.base , pdqu ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "FastHtmlParser failed[%d]" , nret );
			pdqu->status = nret ;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "FastHtmlParser ok" );
		}
	}
	
	if( pdqu->penv->pfuncResponseBodyProc )
	{
		nret = pdqu->penv->pfuncResponseBodyProc( pdqu ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncResponseBodyProc failed[%d]" , nret );
			pdqu->status = nret ;
			if( nret == SIMSPIDER_ERROR_FUNCPROC_INTERRUPT )
				return nret;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "pfuncResponseBodyProc ok" );
		}
	}
	
	return 0;
}
	
static int FinishTask( struct DoneQueueUnit *pdqu , CURL **pp_curl )
{
	int		nret = 0 ;
	
	curl_multi_remove_handle( pdqu->penv->curls , pdqu->curl );
	DebugLog( __FILE__ , __LINE__ , "curl_multi_remove_handle ok , curl[%p]" , pdqu->curl );
	
	if( pp_curl == NULL )
	{
		curl_easy_cleanup( pdqu->curl );
		DebugLog( __FILE__ , __LINE__ , "curl_easy_cleanup ok , curl[%p]" , pdqu->curl );
	}
	else
	{
		(*pp_curl) = pdqu->curl ;
		DebugLog( __FILE__ , __LINE__ , "restore ok , curl[%p]" , (*pp_curl) );
	}
	pdqu->curl = NULL ;
	
	if( pdqu->penv->request_delay > 0 )
	{
#if ( defined _WIN32 )
		Sleep( pdqu->penv->request_delay * 1000 );
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
		sleep( pdqu->penv->request_delay );
#endif
	}
	else if( pdqu->penv->request_delay < 0 )
	{
		unsigned int	seconds ;
		seconds = ( rand() % (-pdqu->penv->request_delay) ) + 1 ;
#if ( defined _WIN32 )
		Sleep( seconds * 1000 );
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
		sleep( seconds );
#endif
	}
	
	if( pdqu->post_url )
	{
		free( pdqu->post_url );
		pdqu->post_url = NULL ;
	}
	
	_FreeDoneQueueUnit( pdqu );
	
	if( pdqu->penv->pfuncUpdateDoneQueueUnitProc )
	{
		nret = pdqu->penv->pfuncUpdateDoneQueueUnitProc( pdqu->penv->done_queue_env , pdqu ) ;
		if( nret < 0 )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncUpdateDoneQueueUnitProc failed[%d]" , nret );
			return SIMSPIDER_ERROR_FUNCPROC_INTERRUPT;
		}
	}
	
	return 0;
}

static unsigned long difftimeval( struct timeval *ptv1 , struct timeval *ptv2 , struct timeval *ptvdiff )
{
	struct timeval	tvdiff ;
	
	tvdiff.tv_sec = ptv2->tv_sec - ptv1->tv_sec ;
	tvdiff.tv_usec = ptv2->tv_usec - ptv1->tv_usec ;
	
	if( ptvdiff )
		memcpy( ptvdiff , & tvdiff , sizeof(struct timeval) );
	
	return tvdiff.tv_sec * 1000*1000 + tvdiff.tv_usec;
}

int SimSpiderGo( struct SimSpiderEnv *penv , char *referer_url , char *url )
{
	int			still_running ;
	CURLMsg			*msg = NULL ;
	int			msgs_in_queue ;
	struct DoneQueueUnit	*pdqu = NULL ;
	
	struct timeval		tv1 , tv2 ;
	unsigned long		diffusec ;
	
	CURL			*curl = NULL ;
	
	int			nret = 0 ;
	
	if( url )
	{
		nret = AppendRequestQueue( penv , referer_url , url , 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "AppendRequestQueue failed[%d]" , nret );
			return nret;
		}
	}
	
	still_running = 0 ;
	nret = FetchTasksFromRequestQueue( penv , & still_running , NULL ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "FetchTasksFromRequestQueue failed[%d]" , nret );
		return nret;
	}
	
	while( still_running )
	{
		do
		{
			nret = curl_multi_perform( penv->curls , & still_running ) ;
		}
		while( nret == CURLM_CALL_MULTI_PERFORM );
		if( nret != CURLM_OK )
		{
			ErrorLog( __FILE__ , __LINE__ , "curl_multi_perform failed[%d]" , nret );
			return SIMSPIDER_ERROR_LIB_MCURL_BASE-nret;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "curl_multi_perform ok , still_running[%d]" , still_running );
		}
		
		if( still_running )
		{
			fd_set		read_fds , write_fds , expection_fds ;
			int		max_fd ;
			long		timeout ;
			struct timeval	tv ;
			
			FD_ZERO( & read_fds );
			FD_ZERO( & write_fds );
			FD_ZERO( & expection_fds );
			
			max_fd = -1 ;
			curl_multi_fdset( penv->curls , & read_fds , & write_fds , & expection_fds , & max_fd );
			DebugLog( __FILE__ , __LINE__ , "curl_multi_fdset max_fd[%d]" , max_fd );
			curl_multi_timeout( penv->curls , & timeout );
			if( timeout == -1 )
			{
				WarnLog( __FILE__ , __LINE__ , "curl_multi_timeout get timeout[%ld]->[1000]ms" , timeout );
				timeout = 1000 ;
			}
			else if( timeout > 60*1000 )
			{
				WarnLog( __FILE__ , __LINE__ , "curl_multi_timeout get timeout[%ld]->[1000]ms" , timeout );
				timeout = 1000 ;
			}
			else
			{
				DebugLog( __FILE__ , __LINE__ , "curl_multi_timeout get timeout[%ld]ms" , timeout );
			}
			
			if( max_fd == -1 )
			{
#if ( defined _WIN32 )
				Sleep( 1000 );
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
				sleep( 1 );
#endif
			}
			else
			{
				if( penv->concurrent_count_automode )
				{
#if ( defined _WIN32 )
					SYSTEMTIME	stNow ;
					GetLocalTime( & stNow );
					SYSTEMTIME2TIMEVAL_USEC( stNow , tv1 );
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
					gettimeofday( & tv1 , NULL );
#endif
				}
				
				tv.tv_sec = timeout / 1000 ;
				tv.tv_usec = ( timeout % 1000 ) * 1000 ;
				nret = select( max_fd + 1 , & read_fds , & write_fds , & expection_fds , & tv ) ;
				if( nret < 0 )
				{
					ErrorLog( __FILE__ , __LINE__ , "select failed[%d] , errno[%d]" , nret , errno );
					return SIMSPIDER_ERROR_SELECT;
				}
				else
				{
					DebugLog( __FILE__ , __LINE__ , "select ok[%d]" , nret );
				}
				
				if( penv->concurrent_count_automode )
				{
#if ( defined _WIN32 )
					SYSTEMTIME	stNow ;
					GetLocalTime( & stNow );
					SYSTEMTIME2TIMEVAL_USEC( stNow , tv2 );
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
					gettimeofday( & tv2 , NULL );
#endif
					diffusec = difftimeval( & tv1 , & tv2 , NULL ) ;
					DebugLog( __FILE__ , __LINE__ , "diffusec/1000[%ld]" , diffusec / 1000 );
					if( diffusec > 500*1000 )
					{
						penv->adjust_concurrent_count++;
					}
					else if( diffusec < 50*1000 )
					{
						penv->adjust_concurrent_count--;
					}
				}
				
				do
				{
					nret = curl_multi_perform( penv->curls , & still_running ) ;
				}
				while( nret == CURLM_CALL_MULTI_PERFORM );
				if( nret != CURLM_OK )
				{
					ErrorLog( __FILE__ , __LINE__ , "curl_multi_perform 2 failed[%d]" , nret );
					return SIMSPIDER_ERROR_LIB_MCURL_BASE-nret;
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "curl_multi_perform 2 ok , still_running[%d]" , still_running );
				}
			}
		}
		
		while(1)
		{
			msg = curl_multi_info_read( penv->curls , & msgs_in_queue ) ;
			if( msg == NULL )
				break;
			
			curl_easy_getinfo( msg->easy_handle , CURLINFO_PRIVATE , & pdqu );
			
			if ( msg->msg == CURLMSG_DONE )
			{
				if( msg->data.result == CURLE_OK )
				{
					long	http_response_code ;
					
					InfoLog( __FILE__ , __LINE__ , "curl_easy_perform ok" );
					
					curl_easy_getinfo( msg->easy_handle , CURLINFO_RESPONSE_CODE , & http_response_code ) ;
					pdqu->status = http_response_code ;
					if( http_response_code == 200 )
					{
						InfoLog( __FILE__ , __LINE__ , "HTTP RESPONSECODE[%ld]" , http_response_code );
					}
					else
					{
						ErrorLog( __FILE__ , __LINE__ , "HTTP RESPONSECODE[%ld]" , http_response_code );
					}
					
					nret = ProcessingTask( pdqu ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "ProcessingTask failed[%d]" , nret );
						FinishTask( pdqu , NULL );
						return nret;
					}
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "curl_easy_perform failed[%d] , errno[%d]" , msg->data.whatever , errno );
					pdqu->status = SIMSPIDER_ERROR_LIB_CURL_BASE-msg->data.result ;
				}
			}
			
			nret = FinishTask( pdqu , & curl ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "FinishTask failed[%d]" , nret );
				return nret;
			}
			
			if( penv->concurrent_count_automode )
			{
				if( penv->adjust_concurrent_count > 0 )
				{
					if( penv->max_concurrent_count < 10000 )
					{
						DebugLog( __FILE__ , __LINE__ , "max_concurrent_count[%ld]++" , penv->max_concurrent_count );
						penv->max_concurrent_count++;
#if CURLMOPT_MAXCONNECTS
						curl_multi_setopt( penv->curls , CURLMOPT_MAXCONNECTS , penv->max_concurrent_count );
#endif
					}
					
					penv->adjust_concurrent_count = 0 ;
				}
				else if( penv->adjust_concurrent_count < 0 )
				{
					if( penv->max_concurrent_count > 1 )
					{
						DebugLog( __FILE__ , __LINE__ , "max_concurrent_count[%ld]--" , penv->max_concurrent_count );
						penv->max_concurrent_count--;
#if CURLMOPT_MAXCONNECTS
						curl_multi_setopt( penv->curls , CURLMOPT_MAXCONNECTS , penv->max_concurrent_count );
#endif
					}
					
					penv->adjust_concurrent_count = 0 ;
				}
			}
			
			nret = FetchTasksFromRequestQueue( penv , & still_running , & curl ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "FetchTasksFromRequestQueue failed[%d]" , nret );
				return nret;
			}
			
			if( curl )
			{
				DebugLog( __FILE__ , __LINE__ , "curl_easy_cleanup ok , curl[%p]" , curl );
				curl_easy_cleanup( curl );
				curl = NULL ;
			}
		}
	}
	
	if( penv->pfuncTravelDoneQueueProc )
	{
		char		url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
		
		memset( url , 0x00 , sizeof(url) );
		nret = TravelHashContainer( & (penv->done_queue) , (unsigned char *) url , sizeof(url) , penv->pfuncTravelDoneQueueProc , NULL );
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "TravelHashContainer failed[%d]" , nret );
			return SIMSPIDER_ERROR_INTERNAL;
		}
	}
	
	return nret;
}

