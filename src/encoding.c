#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iconv.h"

#define MAXLEN_XMLCONTENT	100 * 1024

char *ConvertContentEncodingEx( char *encFrom , char *encTo , char *inptr , int *inptrlen , char *outptr , int *outptrlen )
{
	iconv_t		ic ;
	
	int		ori_outptrlen = 0 ;
	
	static char	outbuf[ MAXLEN_XMLCONTENT + 1 ]; /* 如果用户没有分配内存，则使用静态缓冲区 */
	size_t		outbuflen ; /* 静态缓冲区长度 */
	
	char		*pin = NULL ; /* 输入字符串指针 */
	size_t		inlen ; /* 输入字符串长度 */
	char		*pout = NULL ; /* 输出字符串指针 */
	size_t		*poutlen = NULL ; /* 输出字符串长度地址 */
	
	int		nret ;
	
	/* 打开iconv_t对象 */
	ic = iconv_open( encTo , encFrom ) ;
	if( ic == (iconv_t)-1 )
	{
		 return NULL;
	}
	nret = iconv( ic , NULL , NULL , NULL , NULL ) ;
	
	/* 输入输出指针赋值 */
	pin = inptr ;
	if( inptrlen )
	{
		inlen = (*inptrlen) ;
	}
	else
	{
		inlen = strlen((char*)inptr) + 1 ;
	}
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
	
	/* 编码转换 */
	nret = iconv( ic , (char **) & pin , & inlen , (char **) & pout , poutlen );
	iconv_close( ic ); /* 关闭iconv_t对象 */
	if( nret == -1)
		 return NULL;
	
	/* 返回 */
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

