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

#ifndef MAXLEN_FILENAME
#define MAXLEN_FILENAME				256
#endif

extern char    *__SIMSPIDER_VERSION ;

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

struct SimSpiderEnv ;

int InitSimSpiderEnv( struct SimSpiderEnv **ppenv );
void CleanSimSpiderEnv( struct SimSpiderEnv **ppenv );
void ResetSimSpiderEnv( struct SimSpiderEnv *penv );

#define SIMSPIDER_DEFAULT_VALIDFILENAMEEXTENSION	"htm html"
typedef int funcResponseHeaderProc( char *url , char *data , long *p_data_len );
typedef int funcResponseBodyProc( char *url , char *data , long *p_data_len );
typedef int funcParserHtmlNodeProc( int type , char *xpath , int xpath_len , int xpath_size , char *node , int node_len , char *properties , int properties_len , char *content , int content_len , void *p );
typedef void funcTravelDoneQueueProc( char *key , void *value , long value_len , void *pv );

void SetValidFileExtname( struct SimSpiderEnv *penv , char *valid_file_extname );
void AllowEmptyFileExtname( struct SimSpiderEnv *penv , int allow_empty_file_extname );
void AllowRunOutofWebsite( struct SimSpiderEnv *penv , int allow_runoutof_website );
void SetMaxRecursiveDepth( struct SimSpiderEnv *penv , long max_recursive_depth );
void SetCertificateFilename( struct SimSpiderEnv *penv , char *cert_pathfilename_format , ... );

void SetResponseHeaderProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseHeaderProc );
void SetResponseBodyProc( struct SimSpiderEnv *penv , funcResponseHeaderProc *pfuncResponseBodyProc );
void SetParserHtmlNodeProc( struct SimSpiderEnv *penv , funcParserHtmlNodeProc *pfuncParserHtmlNodeProc );
void SetTravelDoneQueueProc( struct SimSpiderEnv *penv , funcTravelDoneQueueProc *pfuncTravelDoneQueueProc );

void SetEntryUrls( struct SimSpiderEnv *penv , char **urls , char **custom_header , char **post_data );
int SimSpiderGo( struct SimSpiderEnv *penv );

char *GetDoneQueueUnitUrl( void *pdqu );
int GetDoneQueueUnitStatus( void *pdqu );
long GetDoneQueueUnitRecursiveDepth( void *pdqu );

char *ConvertContentEncodingEx( char *encFrom , char *encTo , char *inptr , int *inptrlen , char *outptr , int *outptrlen );
char *ConvertContentEncoding( char *encFrom , char *encTo , char *inptr );

#ifdef __cplusplus
}
#endif

#endif

