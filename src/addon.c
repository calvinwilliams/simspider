#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********* iconv *********/

_WINDLL_FUNC char *ConvertContentEncodingEx( char *encFrom , char *encTo , char *inptr , int *inptrlen , char *outptr , int *outptrlen );
_WINDLL_FUNC char *ConvertContentEncoding( char *encFrom , char *encTo , char *inptr );

#include "iconv.h"

#define MAXLEN_XMLCONTENT	100 * 1024

char *ConvertContentEncodingEx( char *encFrom , char *encTo , char *inptr , int *inptrlen , char *outptr , int *outptrlen )
{
	iconv_t		ic ;
	
	size_t		inlen_bak = 0 ;
	int		ori_outptrlen = 0 ;
	
	static char	outbuf[ MAXLEN_XMLCONTENT + 1 ];
	size_t		outbuflen ;
	
	char		*pin = NULL ;
	size_t		inlen ;
	char		*pout = NULL ;
	size_t		*poutlen = NULL ;
	
	int		nret ;
	
	ic = iconv_open( encTo , encFrom ) ;
	if( ic == (iconv_t)-1 )
	{
		 return NULL;
	}
	nret = iconv( ic , NULL , NULL , NULL , NULL ) ;
	
	pin = inptr ;
	if( inptrlen )
	{
		inlen = (*inptrlen) + 1 ;
	}
	else
	{
		inlen = strlen((char*)inptr) + 1 ;
	}
	inlen_bak = inlen ;
	if( outptr )
	{
		memset( outptr , 0x00 , (*outptrlen) );
		if( inptr == NULL )
			return outptr;
		
		pout = outptr ;
		poutlen = (size_t*)outptrlen ;

		ori_outptrlen = (*outptrlen) ;
	}
	else
	{
		memset( outbuf , 0x00 , sizeof(outbuf) );
		outbuflen = MAXLEN_XMLCONTENT ;
		if( inptr == NULL )
			return outbuf;
		
		pout = outbuf ;
		poutlen = & outbuflen ;
	}
	
	nret = iconv( ic , (char **) & pin , & inlen , (char **) & pout , poutlen );
	iconv_close( ic );
	if( nret == -1 || inlen > 0 )
		return NULL;
	
	if( outptr )
	{
		(*outptrlen) = ori_outptrlen - (*poutlen) ;
		return pout - (*outptrlen) ;
	}
	else
	{
		return outbuf;
	}
}

char *ConvertContentEncoding( char *encFrom , char *encTo , char *inptr )
{
	return ConvertContentEncodingEx( encFrom , encTo , inptr , NULL , NULL , NULL );
}

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

int IsMatchString(char *pcMatchString, char *pcObjectString, char cMatchMuchCharacters, char cMatchOneCharacters)
{
        int el=strlen(pcMatchString);
        int sl=strlen(pcObjectString);
        char cs,ce;

        int is,ie;
        int last_xing_pos=-1;

        for(is=0,ie=0;is<sl && ie<el;){
                cs=pcObjectString[is];
                ce=pcMatchString[ie];

                if(cs!=ce){
                        if(ce==cMatchMuchCharacters){
                                last_xing_pos=ie;
                                ie++;
                        }else if(ce==cMatchOneCharacters){
                                is++;
                                ie++;
                        }else if(last_xing_pos>=0){
                                while(ie>last_xing_pos){
                                        ce=pcMatchString[ie];
                                        if(ce==cs)
                                                break;
                                        ie--;
                                }

                                if(ie==last_xing_pos)
                                        is++;
                        }else
                                return -1;
                }else{
                        is++;
                        ie++;
                }
        }

        if(pcObjectString[is]==0 && pcMatchString[ie]==0)
                return 0;

        if(pcMatchString[ie]==0)
                ie--;

        if(ie>=0){
                while(pcMatchString[ie])
                        if(pcMatchString[ie++]!=cMatchMuchCharacters)
                                return -2;
        }

        return 0;
}

int CountCharInStringWithLength( char *str , int len , char c )
{
	char	*ptr = NULL ;
	int	l ;
	int	count ;
	
	for( ptr = str , l = 0 , count = 0 ; l < len ; ptr++ , l++ )
	{
		if( (*ptr) == c )
			count++;
	}
	
	return count;
}

int CountCharInString( char *str , char c )
{
	return CountCharInStringWithLength( str , strlen(str) , c );
}

int nstoi( char *base , long len )
{
	char buf[ 20 + 1 ] ;
	if( len <= 0 )
		return 0;
	memset( buf , 0x00 , len + 1 );
	strncpy( buf , base , MIN(sizeof(buf)-1,len) );
	return atoi( buf ) ;
}

long nstol( char *base , long len )
{
	char buf[ 40 + 1 ] ;
	if( len <= 0 )
		return 0;
	memset( buf , 0x00 , len + 1 );
	strncpy( buf , base , MIN(sizeof(buf)-1,len) );
	return atol( buf ) ;
}

float nstof( char *base , long len )
{
	char buf[ 40 + 1 ] ;
	if( len <= 0 )
		return 0.0;
	memset( buf , 0x00 , len + 1 );
	strncpy( buf , base , MIN(sizeof(buf)-1,len) );
	return (float)atof( buf ) ;
}

double nstolf( char *base , long len )
{
	char buf[ 80 + 1 ] ;
	if( len <= 0 )
		return 0.00;
	memset( buf , 0x00 , len + 1 );
	strncpy( buf , base , MIN(sizeof(buf)-1,len) );
	return atof( buf ) ;
}

void EraseGB18030( char *str )
{
	unsigned char	*upc = NULL ;
	
	for( upc = (unsigned char *)str ; *upc ; upc++ )
	{
		if( *(upc) > 127 )
		{
			if(
				0xB0 <= *(upc) && *(upc) <= 0xF7
				&&
				0xA1 <= *(upc+1) && *(upc+1) <= 0xFE
			)
			{
				upc++;
			}
			else
			{
				memcpy( (char*)upc , "G8" , 2 );
				upc++;
			}
		}
	}
	
	return;
}

int ConvertBodyEncodingEx( struct DoneQueueUnit *pdqu , char *from_encoding , char *to_encoding )
{
	struct SimSpiderBuf	*buf = NULL ;
	char			*inptr = NULL ;
	int			inlen ;
	char			*outptr = NULL ;
	int			outlen ;
	
	int			nret = 0 ;
	
	buf = GetSimSpiderEnvBodyBuffer( pdqu ) ;
	
	inlen = buf->len ;
	inptr = buf->base ;
	outlen = inlen * 2 ;
	outptr = (char*)malloc( outlen + 1 ) ;
	if( outptr == NULL )
	{
		return -1;
	}
	memset( outptr , 0x00 , outlen + 1 );
	
	if( ConvertContentEncodingEx( "UTF-8" , "GB18030" , inptr , & inlen , outptr , & outlen ) == NULL )
	{
		free( outptr );
		return -2;
	}
	
	if( outlen > inlen )
	{
		nret = ReallocBodyBuffer( pdqu , outlen + 1 ) ;
		if( nret )
		{
			free( outptr );
			return -3;
		}
		
		buf = GetSimSpiderEnvBodyBuffer( pdqu ) ;
	}
	
	strcpy( buf->base , outptr );
	buf->len = outlen ;
	
	free( outptr );
	
	return 0;
}

long _GetFileSize(char *filename)
{
	struct stat stat_buf;
	int ret;

	ret=stat(filename,&stat_buf);
	
	if( ret == -1 )
		return -1;
	
	return stat_buf.st_size;
}

int ReadEntireFile( char *filename , char *mode , char *buf , long *bufsize )
{
	FILE	*fp = NULL ;
	/* int	ch; */
	long	filesize ;
	long	lret ;
	
	if( filename == NULL )
		return -1;
	if( STRCMP( filename , == , "" ) )
		return -1;
	
	filesize = _GetFileSize( filename ) ;
	if( filesize  < 0 )
		return -2;
	
	fp = fopen( filename , mode ) ;
	if( fp == NULL )
		return -3;
	
	if( filesize <= (*bufsize) )
	{
		lret = fread( buf , sizeof(char) , filesize , fp ) ;
		if( lret < filesize )
		{
			fclose( fp );
			return -4;
		}
		
		(*bufsize) = filesize ;
		
		fclose( fp );
		
		return 0;
	}
	else
	{
		lret = fread( buf , sizeof(char) , (*bufsize) , fp ) ;
		if( lret < (*bufsize) )
		{
			fclose( fp );
			return -4;
		}
		
		fclose( fp );
		
		return 1;
	}
}

int ReadEntireFileSafely( char *filename , char *mode , char **pbuf , long *pbufsize )
{
	long	filesize ;
	
	int	nret ;
	
	filesize = _GetFileSize( filename );
	
	(*pbuf) = (char*)malloc( filesize + 1 ) ;
	if( (*pbuf) == NULL )
		return -1;
	memset( (*pbuf) , 0x00 , filesize + 1 );
	
	nret = ReadEntireFile( filename , mode , (*pbuf) , & filesize ) ;
	if( nret )
	{
		free( (*pbuf) );
		(*pbuf) = NULL ;
		return nret;
	}
	else
	{
		if( pbufsize )
			(*pbufsize) = filesize ;
		return 0;
	}
}

char *StringNoEnter( char *str )
{
	char	*ptr = NULL ;
	
	if( str == NULL )
		return NULL;
	
	for( ptr = str + strlen(str) - 1 ; ptr >= str ; ptr-- )
	{
		if( (*ptr) == '\r' || (*ptr) == '\n' )
			(*ptr) = '\0' ;
		else
			break;
	}
	
	return str;
}

int ClearRight( char *str )
{
	char *pc = NULL ;
	long len;
	
	if( str == NULL )
		return -1;
	
	if( *str == '\0' )
		return -2;
	
	len = strlen( str ) ;
	pc = str + len - 1 ;
	while(1)
	{
		if
		(
			*pc == ' '
			||
			*pc == '\t'
			||
			*pc == '\r'
			||
			*pc == '\n'
		)
		{
			;
		}
		else if
		(
			pc > str
			&&
			(
				(unsigned char)(*pc) == 0xA1
				&&
				(unsigned char)(*(pc-1)) == 0xA1
			)
		)
		{
			pc -= 1 ;
		}
		else
		{
			*( pc + 1 ) = '\0' ;
			
			break;
		}
		
		if( pc == str )
		{
			(*pc) = '\0' ;
			
			break;
		}
		
		pc--;
	}
	
	return 0;
}

int ClearLeft( char *str )
{
	char *pc = NULL ;
	long l;
	long len = strlen( str ) ;
	
	if( str == NULL )
		return -1;
	
	if( *str == '\0' )
		return -2;
	
	pc = str ;
	while(1)
	{
		if
		(
			*pc == ' '
			||
			*pc == '\t'
			||
			*pc == '\r'
			||
			*pc == '\n'
		)
		{
			;
		}
		else if
		(
			pc < str + len - 1
			&&
			(
				(unsigned char)(*pc) == 0xA1
				&&
				(unsigned char)(*(pc+1)) == 0xA1
			)
		)
		{
			pc += 1 ;
		}
		else
		{
			break;
		}
		
		pc++;
		
		if( pc == str + len )
			break;
	}
	
	if( pc != str )
	{
		len = strlen( pc );
		for( l = 0 ; l < len + 1 ; l++ )
			*( str + l ) = *( pc + l ) ;
	}
	
	return 0;
}

int DeleteChar( char *str , char ch )
{
	char *p = NULL;
	
	if( str == NULL )
		return -1;
	
	while(1)
	{
		p = strchr( str , ch ) ;
		if( p == NULL )
			break;
		memmove( p , p+1 , strlen(p+1)+1 );
	}
	
	return 0;
}

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
				nret = AppendRequestQueue( pdqu->penv , pdqu->url , url , url_len , pdqu->recursive_depth + 1 ) ;
				if( nret )
				{
					ErrorLog( __FILE__ , __LINE__ , "AppendRequestQueue failed[%d]" , nret );
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

