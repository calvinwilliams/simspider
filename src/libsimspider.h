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
#define SIMSPIDER_MAXLEN_URL			2048
#define SIMSPIDER_VALID_FILE_EXTNAME_SET	1024

#define SIMSPIDER_INFO_OK			0
#define SIMSPIDER_ERROR_ALLOC			-901
#define SIMSPIDER_ERROR_INTERNAL		-903
#define SIMSPIDER_ERROR_URL_TOOLONG		-103
#define SIMSPIDER_ERROR_FUNCPROC		-905
#define SIMSPIDER_ERROR_PARSEHTML		-907
#define SIMSPIDER_ERROR_LIB_LOGC		-701
#define SIMSPIDER_ERROR_LIB_MEMQUEUE		-702
#define SIMSPIDER_ERROR_LIB_HASHX		-703

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

#define SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION	"htm html"
#define SIMSPIDER_NO_PASREHTML				0x01
typedef int funcRequestHeaderProc( struct SimSpiderEnv *penv );
typedef int funcRequestBodyProc( struct SimSpiderEnv *penv );
typedef int funcResponseHeaderProc( struct SimSpiderEnv *penv );
typedef int funcResponseBodyProc( struct SimSpiderEnv *penv );
typedef int funcParseHtmlNodeProc( int type , char *xpath , int xpath_len , int xpath_size , char *node , int node_len , char *properties , int properties_len , char *content , int content_len , void *p );
typedef void funcTravelDoneQueueProc( char *key , void *value , long value_len , void *pv );

void SetValidFileExtnameSet( struct SimSpiderEnv *penv , char *valid_file_extname_set );
void AllowEmptyFileExtname( struct SimSpiderEnv *penv , int allow_empty_file_extname );
void AllowRunOutofWebsite( struct SimSpiderEnv *penv , int allow_runoutof_website );
void SetMaxRecursiveDepth( struct SimSpiderEnv *penv , long max_recursive_depth );
void SetCertificateFilename( struct SimSpiderEnv *penv , char *cert_pathfilename_format , ... );
void SetRequestDelay( struct SimSpiderEnv *penv , long seconds );

void SetRequestHeaderProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestHeaderProc );
void SetRequestBodyProc( struct SimSpiderEnv *penv , funcRequestHeaderProc *pfuncRequestBodyProc );
void SetResponseHeaderProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseHeaderProc );
void SetResponseBodyProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseBodyProc );
void SetParseHtmlNodeProc( struct SimSpiderEnv *penv , funcParseHtmlNodeProc *pfuncParseHtmlNodeProc );
void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc );

int SimSpiderGo( struct SimSpiderEnv *penv , char **urls , char **custom_header , char **post_data );

char *GetSimSpiderEnvUrl( struct SimSpiderEnv *penv );
CURL *GetSimSpiderEnvCurl( struct SimSpiderEnv *penv );
struct SimSpiderBuf *GetSimSpiderEnvHeaderBuffer( struct SimSpiderEnv *penv );
struct SimSpiderBuf *GetSimSpiderEnvBodyBuffer( struct SimSpiderEnv *penv );
void SetSimSpiderEnvCustomDataPtr( struct SimSpiderEnv *penv , void *p );
void *GetSimSpiderEnvCustomDataPtr( struct SimSpiderEnv *penv );
struct DoneQueueUnit *GetSimSpiderEnvDoneQueueUnit( struct SimSpiderEnv *penv );

char *GetDoneQueueUnitUrl( struct DoneQueueUnit *pdqu );
int GetDoneQueueUnitStatus( struct DoneQueueUnit *pdqu );
long GetDoneQueueUnitRecursiveDepth( struct DoneQueueUnit *pdqu );

int AppendRequestInfo( struct SimSpiderEnv *penv , char *url , int url_len , char *custom_header , char *post_data , long recursive_depth );

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

