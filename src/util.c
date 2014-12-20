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

