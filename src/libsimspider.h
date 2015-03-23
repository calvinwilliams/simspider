/*
 * simspider - Net Spider Engine
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

#if ( defined _WIN32 )
#define SYSTEMTIME2TIMEVAL_USEC(_syst_,_tv_)	(_tv_).tv_usec = (_syst_).wMilliseconds * 1000 ;
#endif

/********* simspider *********/

extern char    *__SIMSPIDER_VERSION ;

#define SIMSPIDER_MAXLEN_FILENAME		256
#define SIMSPIDER_MAXLEN_URL			1024
#define SIMSPIDER_VALID_FILE_EXTNAME_SET	256
#define SIMSPIDER_VALID_HTML_FILE_EXTNAME_SET	256

#define SIMSPIDER_INFO_OK			0
#define SIMSPIDER_ERROR_ALLOC			-11
#define SIMSPIDER_ERROR_INTERNAL		-12
#define SIMSPIDER_ERROR_GETENV			-13
#define SIMSPIDER_ERROR_SELECT			-14
#define SIMSPIDER_ERROR_LIB_MEMQUEUE		-1000
#define SIMSPIDER_ERROR_LIB_HASHX		-2000
#define SIMSPIDER_ERROR_LIB_LOGC		-3000
#define SIMSPIDER_ERROR_LIB_CURL_BASE		-4000
#define SIMSPIDER_ERROR_LIB_MCURL_BASE		-5000
#define SIMSPIDER_ERROR_REQUEST_QUEUE_OVERFLOW	-91
#define SIMSPIDER_ERROR_URL_TOOLONG		-92
#define SIMSPIDER_ERROR_URL_INVALID		-93
#define SIMSPIDER_ERROR_FUNCPROC		-95
#define SIMSPIDER_ERROR_PARSEHTML		-97
#define SIMSPIDER_ERROR_PARSEJSON		-98
#define SIMSPIDER_ERROR_FUNCPROC_INTERRUPT	-99

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

#define SIMSPIDER_DEFAULT_REQUESTQUEUE_SIZE		1*1024*1024
_WINDLL_FUNC int ResizeRequestQueue( struct SimSpiderEnv *penv , long size );

#define SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION	""
#define SIMSPIDER_DEFAULT_VALIDHTMLFILENAMEEXTENSION	"htm html shtml cgi fcgi asp aspx php jsp do action"
#define SIMSPIDER_CONCURRENTCOUNT_AUTO			0

_WINDLL_FUNC void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set );
_WINDLL_FUNC void SetValidHtmlFileExtnameSet( struct SimSpiderEnv *penv , char *valid_html_file_extname_set );
_WINDLL_FUNC void AllowEmptyFileExtname( struct SimSpiderEnv *penv , int allow_empty_file_extname );
_WINDLL_FUNC void AllowRunOutofWebsite( struct SimSpiderEnv *penv , int allow_runoutof_website );
_WINDLL_FUNC void SetMaxRecursiveDepth( struct SimSpiderEnv *penv , long max_recursive_depth );
_WINDLL_FUNC void SetCertificateFilename( struct SimSpiderEnv *penv , char *cert_pathfilename_format , ... );
_WINDLL_FUNC void SetRequestDelay( struct SimSpiderEnv *penv , long seconds );
_WINDLL_FUNC void SetMaxConcurrentCount( struct SimSpiderEnv *penv , long max_concurrent_count );
_WINDLL_FUNC void SetMaxRetryCount( struct SimSpiderEnv *penv , long retry_count );
_WINDLL_FUNC void SetAcceptEncoding( struct SimSpiderEnv *penv , char *accept_encoding );
_WINDLL_FUNC void SetTransferEncoding( struct SimSpiderEnv *penv , char *transfer_encoding );

_WINDLL_FUNC int AppendRequestQueue( struct SimSpiderEnv *penv , char *referer_url , char *url , long depth );

_WINDLL_FUNC int SimSpiderGo( struct SimSpiderEnv *penv , char *referer_url , char *url );

typedef int funcRequestHeaderProc( struct DoneQueueUnit *pdqu );
typedef int funcRequestBodyProc( struct DoneQueueUnit *pdqu );
typedef int funcResponseHeaderProc( struct DoneQueueUnit *pdqu );
typedef int funcResponseBodyProc( struct DoneQueueUnit *pdqu );
typedef void funcTravelDoneQueueProc( unsigned char *key , void *value , long value_len , void *pv );

_WINDLL_FUNC void SetRequestHeaderProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestHeaderProc );
_WINDLL_FUNC void SetRequestBodyProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestBodyProc );
_WINDLL_FUNC void SetResponseHeaderProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseHeaderProc );
_WINDLL_FUNC void SetResponseBodyProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseBodyProc );
_WINDLL_FUNC void EnableHtmlLinkerParser( struct SimSpiderEnv *penv , int enable );
_WINDLL_FUNC void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc );

typedef int funcPushRequestQueueUnitProc( void *request_queue_env , char url[SIMSPIDER_MAXLEN_URL+1] );
typedef int funcTravelRequestQueueUnitProc( void *request_queue_env , char url[SIMSPIDER_MAXLEN_URL+1] );
typedef int funcPopupRequestQueueUnitProc( void *request_queue_env , char url[SIMSPIDER_MAXLEN_URL+1] );

_WINDLL_FUNC void SetRequestQueueEnv( struct SimSpiderEnv *penv , void *request_queue_env );
_WINDLL_FUNC void SetPushRequestQueueUnitProc( struct SimSpiderEnv *penv , funcPushRequestQueueUnitProc *pfuncPushRequestQueueUnitProc );
_WINDLL_FUNC void SetTravelRequestQueueUnitProc( struct SimSpiderEnv *penv , funcTravelRequestQueueUnitProc *pfuncTravelRequestQueueUnitProc );
_WINDLL_FUNC void SetPopupRequestQueueUnitProc( struct SimSpiderEnv *penv , funcPopupRequestQueueUnitProc *pfuncPopupRequestQueueUnitProc );

typedef int funcQueryDoneQueueUnitProc( void *done_queue_env , struct DoneQueueUnit *pdqu );
typedef int funcAddDoneQueueUnitProc( void *done_queue_env , struct DoneQueueUnit *pdqu );
typedef int funcUpdateDoneQueueUnitProc( void *done_queue_env , struct DoneQueueUnit *pdqu );

_WINDLL_FUNC void SetDoneQueueEnv( struct SimSpiderEnv *penv , void *done_queue_env );
_WINDLL_FUNC void SetQueryDoneQueueUnitProc( struct SimSpiderEnv *penv , funcQueryDoneQueueUnitProc *pfuncQueryDoneQueueUnitProc );
_WINDLL_FUNC void SetAddDoneQueueUnitProc( struct SimSpiderEnv *penv , funcAddDoneQueueUnitProc *pfuncAddDoneQueueUnitProc );
_WINDLL_FUNC void SetUpdateDoneQueueUnitProc( struct SimSpiderEnv *penv , funcUpdateDoneQueueUnitProc *pfuncUpdateDoneQueueUnitProc );

_WINDLL_FUNC struct curl_slist *GetCurlHeadListPtr( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC void FreeCurlHeadList1Later( struct DoneQueueUnit *pdqu , struct curl_slist *curlheadlist );

_WINDLL_FUNC void FreeCurlList1Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist1 );
_WINDLL_FUNC void FreeCurlList2Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist2 );
_WINDLL_FUNC void FreeCurlList3Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist3 );

_WINDLL_FUNC char *GetDoneQueueUnitRefererUrl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC char *GetDoneQueueUnitUrl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC int GetDoneQueueUnitStatus( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC void SetDoneQueueUnitStatus( struct DoneQueueUnit *pdqu , int status );
_WINDLL_FUNC long GetDoneQueueUnitRecursiveDepth( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC long GetDoneQueueUnitRetryCount( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC CURL *GetDoneQueueUnitCurl( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC struct SimSpiderBuf *GetDoneQueueUnitHeaderBuffer( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC struct SimSpiderBuf *GetDoneQueueUnitBodyBuffer( struct DoneQueueUnit *pdqu );
_WINDLL_FUNC void SetDoneQueueUnitPrivateDataPtr( struct DoneQueueUnit *pdqu , char *private_data );
_WINDLL_FUNC char *GetDoneQueueUnitPrivateDataPtr( struct DoneQueueUnit *pdqu );

_WINDLL_FUNC unsigned long MoveUnsuccessfulDoneQueueUnitsToRequestQueue( struct SimSpiderEnv *penv );
_WINDLL_FUNC void ResetDoneQueue( struct SimSpiderEnv *penv );

_WINDLL_FUNC void SetSimSpiderPublicDataPtr( struct SimSpiderEnv *penv , void *public_data );
_WINDLL_FUNC void *GetSimSpiderPublicDataPtr( struct DoneQueueUnit *pdqu );

_WINDLL_FUNC int ReallocHeaderBuffer( struct DoneQueueUnit *pdqu , long new_bufsize );
_WINDLL_FUNC int ReallocBodyBuffer( struct DoneQueueUnit *pdqu , long new_bufsize );
_WINDLL_FUNC int CleanSimSpiderBuffer( struct DoneQueueUnit *pdqu );

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

#ifdef __cplusplus
}
#endif

#endif

