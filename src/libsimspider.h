/*
 * simspider - Simple Net Spider
 * author	: calvin
 * email	: calvinwilliams.c@gmail.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_LIBSIMSPIDER_
#define _H_LIBSIMSPIDER_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#if ( defined _WIN32 )
#include <windows.h>
#elif ( defined __unix ) || ( defined _AIX ) || ( defined __linux__ ) || ( defined __hpux )
#include <unistd.h>
#endif

#if ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC		_declspec(dllexport)
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC
#endif
#endif

#include "curl/curl.h"

#ifndef STRCMP
#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#endif

#ifndef STRICMP
#if ( defined _WIN32 )
#define STRICMP(_a_,_C_,_b_) ( stricmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strnicmp(_a_,_b_,_n_) _C_ 0 )
#elif ( defined __unix ) || ( defined __linux )
#define STRICMP(_a_,_C_,_b_) ( strcasecmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strncasecmp(_a_,_b_,_n_) _C_ 0 )
#endif
#endif

#ifndef SNPRINTF
#if ( defined __linux__ ) || ( defined __unix ) || ( defined _AIX )
#define SNPRINTF	snprintf
#define VSNPRINTF	vsnprintf
#elif ( defined _WIN32 )
#define SNPRINTF	_snprintf
#define VSNPRINTF	_vsnprintf
#endif
#endif

/********* simspider *********/

extern char    *__SIMSPIDER_VERSION ;

#define SIMSPIDER_MAXLEN_FILENAME		256
#define SIMSPIDER_MAXLEN_URL			1024
#define SIMSPIDER_VALID_FILE_EXTNAME_SET	256

#define SIMSPIDER_INFO_OK			0
#define SIMSPIDER_ERROR_ALLOC			-11
#define SIMSPIDER_ERROR_INTERNAL		-12
#define SIMSPIDER_ERROR_URL_TOOLONG		-13
#define SIMSPIDER_ERROR_SELECT			-14
#define SIMSPIDER_INFO_DONE			20
#define SIMSPIDER_ERROR_LIB_MEMQUEUE		-31
#define SIMSPIDER_ERROR_LIB_HASHX		-32
#define SIMSPIDER_ERROR_LIB_CURL		-33
#define SIMSPIDER_ERROR_LIB_LOGC		-34
#define SIMSPIDER_ERROR_FUNCPROC		-95
#define SIMSPIDER_ERROR_PARSEHTML		-97
#define SIMSPIDER_ERROR_PARSEJSON		-98

#define SIMSPIDER_INFO_THEN_DO_IT_FOR_DEFAULT	902

struct SimSpiderBuf
{
	long		bufsize ;
	long		len ;
	char		*base ;
} ;

struct SimSpiderEnv ;
struct DoneQueueUnit ;

_WINDLL_FUNC int InitSimSpiderEnv( struct SimSpiderEnv **ppenv , char *log_file_format , ... );
_WINDLL_FUNC void CleanSimSpiderEnv( struct SimSpiderEnv **ppenv );
_WINDLL_FUNC void ResetSimSpiderEnv( struct SimSpiderEnv *penv );

#define SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION	"htm html shtml cgi fcgi asp aspx php jsp do action"

typedef int funcRequestHeaderProc( struct DoneQueueUnit *pdqu );
typedef int funcRequestBodyProc( struct DoneQueueUnit *pdqu );
typedef int funcResponseHeaderProc( struct DoneQueueUnit *pdqu );
typedef int funcResponseBodyProc( struct DoneQueueUnit *pdqu );
typedef void funcTravelDoneQueueProc( char *key , void *value , long value_len , void *pv );

_WINDLL_FUNC void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set );
_WINDLL_FUNC void AllowEmptyFileExtname( struct SimSpiderEnv *penv , int allow_empty_file_extname );
_WINDLL_FUNC void AllowRunOutofWebsite( struct SimSpiderEnv *penv , int allow_runoutof_website );
_WINDLL_FUNC void SetMaxRecursiveDepth( struct SimSpiderEnv *penv , long max_recursive_depth );
_WINDLL_FUNC void SetCertificateFilename( struct SimSpiderEnv *penv , char *cert_pathfilename_format , ... );
_WINDLL_FUNC void SetRequestDelay( struct SimSpiderEnv *penv , long seconds );
_WINDLL_FUNC void SetMaxConcurrentCount( struct SimSpiderEnv *penv , long max_concurrent_count );

_WINDLL_FUNC void SetRequestHeaderProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestHeaderProc );
_WINDLL_FUNC void SetRequestBodyProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestBodyProc );
_WINDLL_FUNC void SetResponseHeaderProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseHeaderProc );
_WINDLL_FUNC void SetResponseBodyProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseBodyProc );
_WINDLL_FUNC void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc );

_WINDLL_FUNC int SimSpiderGo( struct SimSpiderEnv *penv , char **urls );

_WINDLL_FUNC char *GetSimSpiderEnvUrl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC CURL *GetSimSpiderEnvCurl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC struct SimSpiderBuf *GetSimSpiderEnvHeaderBuffer( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC struct SimSpiderBuf *GetSimSpiderEnvBodyBuffer( struct DoneQueueUnit *pdqu );

_WINDLL_FUNC char *GetDoneQueueUnitRefererUrl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC char *GetDoneQueueUnitUrl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC int GetDoneQueueUnitStatus( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC long GetDoneQueueUnitRecursiveDepth( struct DoneQueueUnit *pdqu );

_WINDLL_FUNC int AppendRequestUnit( struct SimSpiderEnv *penv , char *referer_url , char *url , int url_len , long depth );

_WINDLL_FUNC int ReallocHeaderBuffer( struct DoneQueueUnit *pdqu , long new_bufsize );
_WINDLL_FUNC int ReallocBodyBuffer( struct DoneQueueUnit *pdqu , long new_bufsize );
_WINDLL_FUNC int CleanSimSpiderBuffer( struct DoneQueueUnit *pdqu );

_WINDLL_FUNC void FreeCurlList1Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist1 );
_WINDLL_FUNC void FreeCurlList2Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist2 );
_WINDLL_FUNC void FreeCurlList3Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist3 );

/********* util *********/

#ifndef MIN
#define MIN(a, b)       ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a, b)       ((a)>(b)?(a):(b))
#endif

_WINDLL_FUNC int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters);
_WINDLL_FUNC int CountCharInStringWithLength( char *str , int len , char c );
_WINDLL_FUNC int CountCharInString( char *str , char c );

_WINDLL_FUNC int nstoi( char *base , long len );
_WINDLL_FUNC long nstol( char *base , long len );
_WINDLL_FUNC float nstof( char *base , long len );
_WINDLL_FUNC double nstolf( char *base , long len );

_WINDLL_FUNC void EraseGB18030( char *str );
_WINDLL_FUNC int ConvertBodyEncodingEx( struct DoneQueueUnit *pdqu , char *from_encoding , char *to_encoding );

_WINDLL_FUNC long _GetFileSize(char *filename);
_WINDLL_FUNC int ReadEntireFile( char *filename , char *mode , char *buf , long *bufsize );
_WINDLL_FUNC int ReadEntireFileSafely( char *filename , char *mode , char **pbuf , long *pbufsize );

_WINDLL_FUNC char *StringNoEnter( char *str );
_WINDLL_FUNC int ClearRight( char *str );
_WINDLL_FUNC int ClearLeft( char *str );
_WINDLL_FUNC int DeleteChar( char *str , char ch );

/********* iconv *********/

_WINDLL_FUNC char *ConvertContentEncodingEx( char *encFrom , char *encTo , char *inptr , int *inptrlen , char *outptr , int *outptrlen );
_WINDLL_FUNC char *ConvertContentEncoding( char *encFrom , char *encTo , char *inptr );

/********* LOGC *********/

_WINDLL_FUNC void SetLogFile( char *format , ... );
_WINDLL_FUNC void SetLogFileV( char *format , va_list valist );
_WINDLL_FUNC void SetLogLevel( int log_level );

_WINDLL_FUNC int WriteLog( int log_level , char *c_filename , long c_fileline , char *format , ... );
_WINDLL_FUNC int FatalLog( char *c_filename , long c_fileline , char *format , ... );
_WINDLL_FUNC int ErrorLog( char *c_filename , long c_fileline , char *format , ... );
_WINDLL_FUNC int WarnLog( char *c_filename , long c_fileline , char *format , ... );
_WINDLL_FUNC int InfoLog( char *c_filename , long c_fileline , char *format , ... );
_WINDLL_FUNC int DebugLog( char *c_filename , long c_fileline , char *format , ... );

_WINDLL_FUNC int WriteHexLog( int log_level , char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
_WINDLL_FUNC int FatalHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
_WINDLL_FUNC int ErrorHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
_WINDLL_FUNC int WarnHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
_WINDLL_FUNC int InfoHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
_WINDLL_FUNC int DebugHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );

/********* Html Parser Demo Using fasterxml *********/

/*
	AddSkipXmlTag( "meta" );
	AddSkipXmlTag( "br" );
	AddSkipXmlTag( "p" );
	AddSkipXmlTag( "img" );
	AddSkipXmlTag( "image" );
	AddSkipXmlTag( "link" );
	AddSkipXmlTag( "input" );
	
	CleanSkipXmlTags();
	
int CallbackOnXmlProperty( char *xpath , int xpath_len , int xpath_size , char *propname , int propname_len , char *propvalue , int propvalue_len , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	
	int			nret = 0 ;
	
	if( propname_len == 4 && STRNICMP( propname , == , "href" , propname_len ) )
	{
		if( pdqu->penv->max_recursive_depth > 1 && pdqu->recursive_depth >= pdqu->penv->max_recursive_depth )
			return 0;
		
		nret = CheckHttpProtocol( propvalue , propvalue_len ) ;
		if( nret == 0 )
			return 0;
		
		nret = CheckFileExtname( pdqu->penv , propvalue , propvalue_len ) ;
		if( nret )
		{
			char		url[ SIMSPIDER_MAXLEN_URL + 1 ] ;
			long		url_len ;
			
			memset( url , 0x00 , sizeof(url) );
			strcpy( url , pdqu->url );
			nret = FormatNewUrl( pdqu->penv , propvalue , propvalue_len , url ) ;
			if( nret > 0 )
			{
				InfoLog( __FILE__ , __LINE__ , "FormatNewUrl[%.*s][%s] return[%d]" , propvalue_len , propvalue , url , nret );
				return 0;
			}
			else if( nret < 0 )
			{
				ErrorLog( __FILE__ , __LINE__ , "FormatNewUrl[%.*s][%s] failed[%d]" , propvalue_len , propvalue , url , nret );
				return 0;
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
	}
	
	return 0;
}

int ParseHtmlNodeProc( int type , char *xpath , int xpath_len , int xpath_size , char *node , int node_len , char *properties , int properties_len , char *content , int content_len , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	
	int			nret = 0 ;
	
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
	else if( type & FASTERXML_NODE_LEAF )
	{
		DebugLog( __FILE__ , __LINE__ , "LEAF        [%.*s] - [%.*s]" , xpath_len , xpath , content_len , content );
	}
	
	if( pdqu->penv->pfuncParseHtmlNodeProc )
	{
		nret = pdqu->penv->pfuncParseHtmlNodeProc( type , xpath , xpath_len , xpath_size , node , node_len , properties , properties_len , content , content_len , p ) ;
		if( nret )
		{
			return nret;
		}
		else
		{
			return 0;
		}
	}
	
	if( type & FASTERXML_NODE_LEAF )
	{
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
*/

/********* Json Parser Demo Using fasterjson *********/

/*
int ParseJsonNodeProc( int type , char *jpath , int jpath_len , int jpath_size , char *node , int node_len , char *content , int content_len , void *p )
{
	struct DoneQueueUnit	*pdqu = (struct DoneQueueUnit *)p ;
	
	int			nret = 0 ;
	
	if( type & FASTERJSON_NODE_BRANCH )
	{
		if( type & FASTERJSON_NODE_ENTER )
		{
			DebugLog( __FILE__ , __LINE__ , "ENTER-BRANCH[%.*s]" , jpath_len , jpath );
		}
		else if( type & FASTERJSON_NODE_LEAVE )
		{
			DebugLog( __FILE__ , __LINE__ , "LEAVE-BRANCH[%.*s]" , jpath_len , jpath );
		}
	}
	else if( type & FASTERJSON_NODE_LEAF )
	{
		DebugLog( __FILE__ , __LINE__ , "LEAF        [%.*s] - [%.*s]" , jpath_len , jpath , content_len , content );
	}
	
	if( pdqu->penv->pfuncParseJsonNodeProc )
	{
		nret = pdqu->penv->pfuncParseJsonNodeProc( type , jpath , jpath_len , jpath_size , node , node_len , content , content_len , p ) ;
		if( nret )
		{
			return nret;
		}
		else
		{
			return 0;
		}
	}
	
	return 0;
}
*/

#ifdef __cplusplus
}
#endif

#endif

