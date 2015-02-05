/*
 * simspider - Simple Net Spider
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

char	__SIMSPIDER_VERSION_2_1_1[] = "2.1.1" ;
char	*__SIMSPIDER_VERSION = __SIMSPIDER_VERSION_2_1_1 ;

struct SimSpiderEnv
{
	CURLM			*curls ;
	
	char			valid_file_extname_set[ SIMSPIDER_VALID_FILE_EXTNAME_SET + 1 ] ;
	int			allow_empty_file_extname ;
	char			cert_pathfilename[ SIMSPIDER_MAXLEN_FILENAME + 1 ] ;
	int			allow_runoutof_website ;
	long			max_recursive_depth ;
	long			request_delay ;
	long			max_concurrent_count ;
	funcRequestHeaderProc	*pfuncRequestHeaderProc ;
	funcRequestBodyProc	*pfuncRequestBodyProc ;
	funcResponseHeaderProc	*pfuncResponseHeaderProc ;
	funcResponseBodyProc	*pfuncResponseBodyProc ;
	funcTravelDoneQueueProc	*pfuncTravelDoneQueueProc ;
	
	struct MemoryQueue	*request_queue ;
	struct HashContainer	done_queue ;
} ;

struct DoneQueueUnit
{
	char			referer_url [ SIMSPIDER_MAXLEN_URL + 1 ] ;
	char			url [ SIMSPIDER_MAXLEN_URL + 1 ] ;
	int			status ;
	long			recursive_depth ;
	
	struct SimSpiderEnv	*penv ;
	struct QueueBlock	*pqb ;
	CURL			*curl ;
	struct curl_slist	*free_curllist1_later ;
	struct curl_slist	*free_curllist2_later ;
	struct curl_slist	*free_curllist3_later ;
	struct SimSpiderBuf	header ;
	struct SimSpiderBuf	body ;
} ;

static BOOL FreeDoneQueueUnit( void *pv )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)pv ;
	
	if( pdqu )
	{
		if( pdqu->header.base )
		{
			free( pdqu->header.base );
		}
		if( pdqu->body.base )
		{
			free( pdqu->body.base );
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

char *GetSimSpiderEnvUrl( struct DoneQueueUnit *pdqu )
{
	return pdqu->url;
}

CURL *GetSimSpiderEnvCurl( struct DoneQueueUnit *pdqu )
{
	return pdqu->curl;
}

struct SimSpiderBuf *GetSimSpiderEnvHeaderBuffer( struct DoneQueueUnit *pdqu )
{
	return & (pdqu->header);
}

struct SimSpiderBuf *GetSimSpiderEnvBodyBuffer( struct DoneQueueUnit *pdqu )
{
	return & (pdqu->body);
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
		return -2;
	
	new_base = (char*)realloc( pdqu->header.base , new_bufsize ) ;
	if( new_base == NULL )
		return -1;
	memset( new_base + pdqu->header.len , 0x00 , new_bufsize - pdqu->header.len );
	
	pdqu->header.base = new_base ;
	pdqu->header.bufsize = new_bufsize ;
	
	return 0;
}

int ReallocBodyBuffer( struct DoneQueueUnit *pdqu , long new_bufsize )
{
	char	*new_base = NULL ;
	
	if( new_bufsize <= pdqu->body.bufsize )
		return -2;
	
	new_base = (char*)realloc( pdqu->body.base , new_bufsize ) ;
	if( new_base == NULL )
		return -1;
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
			return SIMSPIDER_ERROR_LIB_LOGC;
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
	
	curl_global_init( CURL_GLOBAL_DEFAULT );
	(*ppenv)->curls = curl_multi_init() ;
	
	SetValidFileExtnameSet( (*ppenv) , SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION );
	AllowEmptyFileExtname( (*ppenv) , 1 );
	AllowRunOutofWebsite( (*ppenv) , 0 );
	SetMaxConcurrentCount( (*ppenv) , 1 );
	
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

void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set )
{
	memset( penv->valid_file_extname_set , 0x00 , sizeof(penv->valid_file_extname_set) );
	SNPRINTF( penv->valid_file_extname_set , sizeof(penv->valid_file_extname_set)-1 , " %s " , valid_file_extname_set );
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
	penv->max_concurrent_count = max_concurrent_count ;
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
			return -103;
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
			return -103;
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
			return -1;
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
					return -103;
				SNPRINTF( url + url_len , SIMSPIDER_MAXLEN_URL - url_len - 1 , "/%.*s" , propvalue_len , propvalue );
			}
			else
			{
				ptr = strrnchr( url , ptr - url , '/' ) ;
				if( ptr == NULL )
					return 1;
				if( ptr-url+propvalue_len > SIMSPIDER_MAXLEN_URL )
					return -103;
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

static int CheckFileExtname( struct SimSpiderEnv *penv , char *propvalue , int propvalue_len )
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
	if( strstr( penv->valid_file_extname_set , file_extname ) )
		return 1;
	
	return 0;
}

int AppendRequestUnit( struct SimSpiderEnv *penv , char *referer_url , char *url , int url_len , long depth )
{
	struct DoneQueueUnit	*pdqu = NULL ;
	
	int			nret = 0 ;
	
	pdqu = AllocDoneQueueUnit( referer_url , url , 0 , depth ) ;
	if( pdqu == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "AllocDoneQueueUnit failed errno[%d]" , errno );
		return SIMSPIDER_ERROR_ALLOC;
	}
	
	nret = PutHashItem( & (penv->done_queue) , url , (void*) pdqu , sizeof(struct DoneQueueUnit) , & FreeDoneQueueUnit , HASH_PUTMODE_ADD ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "PutHashItem failed[%d] errno[%d]" , nret , errno );
		FreeDoneQueueUnit( pdqu );
		return -1;
	}
	
	nret = AddQueueBlock( penv->request_queue , url , url_len + 1 , NULL ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "AddQueueBlock[%s] failed[%d]" , url , nret );
		return -1;
	}
	else
	{
		DebugLog( __FILE__ , __LINE__ , "AddQueueBlock[%s][%ld] ok" , url , depth );
	}
	
	return 0;
}

#define FIND_HREF		" href="

char *strcasestr(const char *haystack, const char *needle);

int HtmlLinkParser( char *buffer , struct DoneQueueUnit *pdqu )
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
		
		nret = CheckFileExtname( pdqu->penv , propvalue , propvalue_len ) ;
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
			
			nret = GetHashItemPtr( & (pdqu->penv->done_queue) , url , NULL , NULL ) ;
			if( nret == HASH_RETCODE_ERROR_KEY_NOT_EXIST )
			{
				nret = AppendRequestUnit( pdqu->penv , pdqu->url , url , url_len , pdqu->recursive_depth + 1 ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "AppendRequestUnit failed[%d]" , nret );
					return -1;
				}
			}
			else if( nret < 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "GetHashItemPtr failed[%d] errno[%d]" , nret , errno );
				return -1;
			}
		}
		
		propvalue += propvalue_len ;
	}
	
	return 0;
}

size_t CurlResponseHeaderProc( char *buffer , size_t size , size_t nmemb , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	int			nret = 0 ;
	
	if( (long)(size*nmemb) > pdqu->header.bufsize-1 - pdqu->header.len )
	{
		nret = ReallocHeaderBuffer( pdqu , pdqu->header.len + size*nmemb + 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "ReallocBodyBuffer failed[%d] errno[%d]" , nret , errno );
			return -1;
		}
	}
	memcpy( pdqu->header.base + pdqu->header.len , buffer , size*nmemb );
	pdqu->header.len += size*nmemb ;
	return size*nmemb;
}

size_t CurlResponseBodyProc( char *buffer , size_t size , size_t nmemb , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	int			nret = 0 ;
	
	if( (long)(size*nmemb) > pdqu->body.bufsize-1 - pdqu->body.len )
	{
		nret = ReallocBodyBuffer( pdqu , pdqu->body.len + size*nmemb + 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "ReallocBodyBuffer failed[%d] errno[%d]" , nret , errno );
			return -1;
		}
	}
	memcpy( pdqu->body.base + pdqu->body.len , buffer , size*nmemb );
	pdqu->body.len += size*nmemb ;
	return size*nmemb;
}

size_t CurlDebugProc( CURL *curl , curl_infotype type , char *buffer , size_t size , void *p )
{
	DebugLog( __FILE__ , __LINE__ , "[%.*s]" , size , buffer );
	return 0;
}

static int FetchTasksFromRequestQueue( struct SimSpiderEnv *penv )
{
	int			i ;
	struct QueueBlock	*pqb = NULL ;
	struct DoneQueueUnit	*pdqu = NULL ;	
	
	int			nret = 0 ;
	
	for( i = 0 ; i < penv->max_concurrent_count ; i++ )
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
		
		CleanSimSpiderBuffer( pdqu );
		
		pdqu->pqb = pqb ;
		pdqu->penv = penv ;
		
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
		
		curl_easy_setopt( pdqu->curl , CURLOPT_COOKIEFILE , "" );
		
		curl_easy_setopt( pdqu->curl , CURLOPT_URL , pdqu->url );
		curl_easy_setopt( pdqu->curl , CURLOPT_TCP_NODELAY , 1L );
		curl_easy_setopt( pdqu->curl , CURLOPT_FOLLOWLOCATION , 1L );
		curl_easy_setopt( pdqu->curl , CURLOPT_VERBOSE , 1L );
		curl_easy_setopt( pdqu->curl , CURLOPT_DEBUGFUNCTION , & CurlDebugProc );
		curl_easy_setopt( pdqu->curl , CURLOPT_DEBUGDATA , NULL );
		
		if( penv->pfuncResponseHeaderProc )
		{
			curl_easy_setopt( pdqu->curl , CURLOPT_HEADER , 0 );
			curl_easy_setopt( pdqu->curl , CURLOPT_HEADERFUNCTION , & CurlResponseHeaderProc );
			curl_easy_setopt( pdqu->curl , CURLOPT_HEADERDATA , pdqu );
		}
		
		curl_easy_setopt( pdqu->curl , CURLOPT_WRITEFUNCTION , & CurlResponseBodyProc );
		curl_easy_setopt( pdqu->curl , CURLOPT_WRITEDATA , pdqu );
		
		if( STRNCMP( pdqu->url , == , "http:" , 5 ) )
		{
			InfoLog( __FILE__ , __LINE__ , "--- [%s] ------------------ HTTP" , pdqu->url );
		}
		else if( STRNCMP( pdqu->url , == , "https:" , 6 ) )
		{
			InfoLog( __FILE__ , __LINE__ , "--- [%s] ------------------ HTTPS" , pdqu->url );
			curl_easy_setopt( pdqu->curl , CURLOPT_SSL_VERIFYPEER , 1L );
			curl_easy_setopt( pdqu->curl , CURLOPT_CAINFO , penv->cert_pathfilename );
			curl_easy_setopt( pdqu->curl , CURLOPT_SSL_VERIFYHOST , 1L );
		}
		
		if( penv->pfuncRequestHeaderProc )
		{
			penv->pfuncRequestHeaderProc( pdqu );
		}
		
		if( penv->pfuncRequestBodyProc )
		{
			penv->pfuncRequestBodyProc( pdqu );
		}
		
		curl_easy_setopt( pdqu->curl , CURLOPT_PRIVATE , pdqu );
		curl_multi_add_handle( penv->curls , pdqu->curl );
		
		RemoveQueueBlock( pdqu->penv->request_queue , pdqu->pqb );
	}
	
	return 0;
}

static int FinishTask( struct DoneQueueUnit *pdqu )
{
	int		nret = 0 ;
	
	if( pdqu->penv->pfuncResponseHeaderProc )
	{
		nret = pdqu->penv->pfuncResponseHeaderProc( pdqu ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncResponseHeaderProc failed[%d]" , nret );
			pdqu->status = SIMSPIDER_ERROR_FUNCPROC ;
			return pdqu->status;
		}
	}
	
	nret = HtmlLinkParser( pdqu->body.base , pdqu ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "FastHtmlParser failed[%d]" , nret );
		pdqu->status = SIMSPIDER_ERROR_PARSEHTML ;
		return pdqu->status;
	}
	else
	{
		DebugLog( __FILE__ , __LINE__ , "FastHtmlParser ok" );
	}
	
	if( pdqu->penv->pfuncResponseBodyProc )
	{
		nret = pdqu->penv->pfuncResponseBodyProc( pdqu ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "pfuncResponseBodyProc failed[%d]" , nret );
			pdqu->status = SIMSPIDER_ERROR_FUNCPROC ;
			return pdqu->status;
		}
		else
		{
			DebugLog( __FILE__ , __LINE__ , "pfuncResponseBodyProc ok" );
		}
	}
	
	pdqu->status = SIMSPIDER_INFO_DONE ;
	
	return 0;
}
	
static int RemoveTaskFromRequestQueue( struct DoneQueueUnit *pdqu )
{
	curl_multi_remove_handle( pdqu->penv->curls , pdqu->curl );
	DebugLog( __FILE__ , __LINE__ , "curl_easy_cleanup ok , curl[%p]\n" , pdqu->curl );
	curl_easy_cleanup( pdqu->curl );
	
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
	
	return 0;
}

int SimSpiderGo( struct SimSpiderEnv *penv , char **entry_urls )
{
	char			url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
	int			url_len ;
	
	int			still_running ;
	CURLMsg			*msg = NULL ;
	int			msgs_in_queue ;
	struct DoneQueueUnit	*pdqu = NULL ;
	
	int			nret = 0 ;
	
	if( penv == NULL )
		return 0;
	
	for( ; entry_urls && (*entry_urls) ; entry_urls++ )
	{
		if( STRNICMP( *entry_urls , != , "http:" , 4 ) )
			SNPRINTF( url , sizeof(url) , "http://%s" , *entry_urls );
		else
			SNPRINTF( url , sizeof(url) , "%s" , *entry_urls );
		
		url_len = strlen( url ) ;
		if( url[url_len-1] != '/' )
			memcpy( url + url_len , "/" , 2 );
		
		nret = AppendRequestUnit( penv , "" , url , url_len , 1 ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "AppendRequestUnit failed[%d]" , nret );
			return -1;
		}
	}
	
	nret = FetchTasksFromRequestQueue( penv ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "FetchTasksFromRequestQueue failed[%d]" , nret );
		return nret;
	}
	
	still_running = -1 ;
	while( still_running )
	{
		while( CURLM_CALL_MULTI_PERFORM == curl_multi_perform( penv->curls , & still_running ) );
		
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
			curl_multi_timeout( penv->curls , & timeout );
			if( timeout == -1 )
				timeout = 100 ;
			if( max_fd == -1 )
			{
#if ( defined _WIN32 )
				Sleep( timeout );
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
				sleep( timeout / 1000 );
#endif
			}
			else
			{
				tv.tv_sec = timeout / 1000 ;
				tv.tv_usec = ( timeout % 1000 ) * 1000 ;
				nret = select( max_fd + 1 , & read_fds , & write_fds , & expection_fds , & tv ) ;
				if( nret < 0 )
				{
					ErrorLog( __FILE__ , __LINE__ , "select failed[%d] , errno[%d]" , nret , errno );
					return SIMSPIDER_ERROR_SELECT;
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
					InfoLog( __FILE__ , __LINE__ , "curl_easy_perform ok" );
					
					nret = FinishTask( pdqu ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "FinishTask failed[%d]" , nret );
					}
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "curl_easy_perform failed[%d] , errno[%d]" , msg->data.whatever , errno );
					pdqu->status = SIMSPIDER_ERROR_LIB_CURL ;
				}
			}
			
			nret = RemoveTaskFromRequestQueue( pdqu ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "RemoveTaskFromRequestQueue failed[%d]" , nret );
				return nret;
			}
			
			nret = FetchTasksFromRequestQueue( penv ) ;
			if( nret )
			{
				ErrorLog( __FILE__ , __LINE__ , "FetchTasksFromRequestQueue failed[%d]" , nret );
				return nret;
			}
			still_running++;
		}
	}
	
	if( penv->pfuncTravelDoneQueueProc )
	{
		char		url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
		
		memset( url , 0x00 , sizeof(url) );
		nret = TravelHashContainer( & (penv->done_queue) , url , sizeof(url) , penv->pfuncTravelDoneQueueProc , NULL );
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "TravelHashContainer failed[%d]" , nret );
			return SIMSPIDER_ERROR_INTERNAL;
		}
	}
	
	return nret;
}

