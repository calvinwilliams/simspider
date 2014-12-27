#include "libsimspider.h"

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
/*
                                加入中文支持 (一个'?'匹配1个汉字)
                                if((unsigned)(cs)>0x7f && is+1>=sl)
                                        is++;
*/
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

