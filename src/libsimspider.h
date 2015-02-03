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
#include <unistd.h>

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

/********* simspider *********/

extern char    *__SIMSPIDER_VERSION ;

#define SIMSPIDER_MAXLEN_FILENAME		256
#define SIMSPIDER_MAXLEN_URL			1024
#define SIMSPIDER_VALID_FILE_EXTNAME_SET	256

#define SIMSPIDER_INFO_OK			0
#define SIMSPIDER_ERROR_ALLOC			-101
#define SIMSPIDER_ERROR_INTERNAL		-102
#define SIMSPIDER_ERROR_URL_TOOLONG		-103
#define SIMSPIDER_ERROR_SELECT			-104
#define SIMSPIDER_ERROR_LIB_MEMQUEUE		-301
#define SIMSPIDER_ERROR_LIB_HASHX		-302
#define SIMSPIDER_ERROR_LIB_CURL		-303
#define SIMSPIDER_ERROR_LIB_LOGC		-304
#define SIMSPIDER_ERROR_FUNCPROC		-905
#define SIMSPIDER_ERROR_PARSEHTML		-907
#define SIMSPIDER_ERROR_PARSEJSON		-908

#define SIMSPIDER_INFO_THEN_DO_IT_FOR_DEFAULT	902

struct SimSpiderBuf
{
	int		bufsize ;
	int		len ;
	char		*base ;
} ;

struct SimSpiderEnv ;
struct DoneQueueUnit ;

int InitSimSpiderEnv( struct SimSpiderEnv **ppenv , char *log_file_format , ... );
void CleanSimSpiderEnv( struct SimSpiderEnv **ppenv );
void ResetSimSpiderEnv( struct SimSpiderEnv *penv );

#define SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION	"htm html shtml cgi fcgi asp aspx php jsp do action"
#define SIMSPIDER_NO_PASREHTML				0x01
#define SIMSPIDER_PARSER_HTML				0
#define SIMSPIDER_PARSER_JSON				1
#define SIMSPIDER_PARSER_FASTHTML			3

typedef int funcRequestHeaderProc( struct DoneQueueUnit *pdqu );
typedef int funcRequestBodyProc( struct DoneQueueUnit *pdqu );
typedef int funcResponseHeaderProc( struct DoneQueueUnit *pdqu );
typedef int funcResponseBodyProc( struct DoneQueueUnit *pdqu );
typedef int funcParseHtmlNodeProc( int type , char *xpath , int xpath_len , int xpath_size , char *node , int node_len , char *properties , int properties_len , char *content , int content_len , void *p );
typedef int funcParseJsonNodeProc( int type , char *jpath , int jpath_len , int jpath_size , char *node , int node_len , char *content , int content_len , void *p );
typedef void funcTravelDoneQueueProc( char *key , void *value , long value_len , void *pv );

void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set );
void AllowEmptyFileExtname( struct SimSpiderEnv *penv , int allow_empty_file_extname );
void AllowRunOutofWebsite( struct SimSpiderEnv *penv , int allow_runoutof_website );
void SetMaxRecursiveDepth( struct SimSpiderEnv *penv , long max_recursive_depth );
void SetCertificateFilename( struct SimSpiderEnv *penv , char *cert_pathfilename_format , ... );
void SetRequestDelay( struct SimSpiderEnv *penv , long seconds );
void SetMaxConcurrentCount( struct SimSpiderEnv *penv , long max_concurrent_count );
void SetResponseBodyParser( struct SimSpiderEnv *penv , int parser );

void SetRequestHeaderProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestHeaderProc );
void SetRequestBodyProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestBodyProc );
void SetResponseHeaderProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseHeaderProc );
void SetResponseBodyProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseBodyProc );
void SetParseHtmlNodeProc( struct SimSpiderEnv *penv , funcParseHtmlNodeProc *pfuncParseHtmlNodeProc );
void SetParseJsonNodeProc( struct SimSpiderEnv *penv , funcParseJsonNodeProc *pfuncParseJsonNodeProc );
void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc );

int SimSpiderGo( struct SimSpiderEnv *penv , char **urls );

char *GetSimSpiderEnvUrl( struct DoneQueueUnit *pdqu );
CURL *GetSimSpiderEnvCurl( struct DoneQueueUnit *pdqu );
struct SimSpiderBuf *GetSimSpiderEnvHeaderBuffer( struct DoneQueueUnit *pdqu );
struct SimSpiderBuf *GetSimSpiderEnvBodyBuffer( struct DoneQueueUnit *pdqu );

char *GetDoneQueueUnitRefererUrl( struct DoneQueueUnit *pdqu );
char *GetDoneQueueUnitUrl( struct DoneQueueUnit *pdqu );
int GetDoneQueueUnitStatus( struct DoneQueueUnit *pdqu );
long GetDoneQueueUnitRecursiveDepth( struct DoneQueueUnit *pdqu );

int AppendRequestInfo( struct SimSpiderEnv *penv , char *referer_url , char *url , int url_len , long depth );

int ReallocHeaderBuffer( struct DoneQueueUnit *pdqu , size_t new_bufsize );
int ReallocBodyBuffer( struct DoneQueueUnit *pdqu , size_t new_bufsize );
int CleanSimSpiderBuffer( struct DoneQueueUnit *pdqu );

void FreeCurlList1Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist1 );
void FreeCurlList2Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist2 );
void FreeCurlList3Later( struct DoneQueueUnit *pdqu , struct curl_slist *curllist3 );

/********* util *********/

#ifndef MIN
#define MIN(a, b)       ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a, b)       ((a)>(b)?(a):(b))
#endif

int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters);
int CountCharInStringWithLength( char *str , int len , char c );
int CountCharInString( char *str , char c );

int nstoi( char *base , long len );
long nstol( char *base , long len );
float nstof( char *base , long len );
double nstolf( char *base , long len );

void EraseGB18030( char *str );
int ConvertBodyEncodingEx( struct DoneQueueUnit *pdqu , char *from_encoding , char *to_encoding );

long _GetFileSize(char *filename);
int ReadEntireFile( char *filename , char *mode , char *buf , long *bufsize );
int ReadEntireFileSafely( char *filename , char *mode , char **pbuf , long *pbufsize );

char *StringNoEnter( char *str );
int ClearRight( char *str );
int ClearLeft( char *str );
int DeleteChar( char *str , char ch );

/********* iconv *********/

char *ConvertContentEncodingEx( char *encFrom , char *encTo , char *inptr , int *inptrlen , char *outptr , int *outptrlen );
char *ConvertContentEncoding( char *encFrom , char *encTo , char *inptr );

/********* LOGC *********/

void SetLogFile( char *format , ... );
void SetLogFileV( char *format , va_list valist );
void SetLogLevel( int log_level );

int WriteLog( int log_level , char *c_filename , long c_fileline , char *format , ... );
int FatalLog( char *c_filename , long c_fileline , char *format , ... );
int ErrorLog( char *c_filename , long c_fileline , char *format , ... );
int WarnLog( char *c_filename , long c_fileline , char *format , ... );
int InfoLog( char *c_filename , long c_fileline , char *format , ... );
int DebugLog( char *c_filename , long c_fileline , char *format , ... );

int WriteHexLog( int log_level , char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
int FatalHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
int ErrorHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
int WarnHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
int InfoHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );
int DebugHexLog( char *c_filename , long c_fileline , char *buf , long buflen , char *format , ... );

#ifdef __cplusplus
}
#endif

#endif

