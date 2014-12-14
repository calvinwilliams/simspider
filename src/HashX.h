#ifndef _INCLUDE_HASHX_H_
#define _INCLUDE_HASHX_H_

/*
** 库名		:       iLibX.HashX
** 库描述	:       哈希函数库 && 算法接口
** 作者		:       calvin
** E-mail	:       
** QQ		:       
** 创建日期时间	:       2013-04-06 通用哈希容器、多维哈希算法
** 更新日期时间	:       2014-12-04 普通哈希算法
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#define BOOLNULL -1
#endif

#define _WINDLL_EXPORT

#define HASH_RETCODE_ERROR_PARAMETER		-7
#define HASH_RETCODE_ERROR_ALLOC		-11
#define HASH_RETCODE_ERROR_INTERNAL		-12
#define HASH_RETCODE_WARN_ALGORITHM_NOT_SUPPORT	-21
#define HASH_RETCODE_ERROR_KEY_EXIST		-41
#define HASH_RETCODE_ERROR_KEY_NOT_EXIST	-42
#define HASH_RETCODE_ERROR_BUFFER_OVERFLOW	-51
#define HASH_RETCODE_INFO_NODE_EXISTED 		61

#define HASH_ALGORITHM_HASH			3 /* 普通哈希算法 */
#define HASH_ALGORITHM_MDHASH			6 /* 多维哈希算法 */

#define HASH_PUTMODE_SET			1
#define HASH_PUTMODE_ADD			2
#define HASH_PUTMODE_REPLACE			4
#define HASH_PUTMODE_DUPLICATE			8

/* 多维哈希算法 */

typedef BOOL funcFreeMDHashNodeProc(void *pv) ;

struct MDHashTree
{
	unsigned long			key_count ;
	unsigned long			total_value_size ;
	
	struct MDHashNode
	{
		void			*a_key[ 256 ] ;
		
		void			*value ;
		long			value_len ;
		funcFreeMDHashNodeProc	*pfuncFreeMDHashNodeProc ;
	} *p_node ;
} ;

_WINDLL_EXPORT int InitMDHashTree( struct MDHashTree *pmdht );
_WINDLL_EXPORT int CleanMDHashTree( struct MDHashTree *pmdht );

_WINDLL_EXPORT int PutMDHashNode( struct MDHashTree *pmdht , char *key , void *value , long value_len , BOOL (*pfuncFreeMDHashNodeProc)(void *pv) , int mode );
_WINDLL_EXPORT int GetMDHashNodePtr( struct MDHashTree *pmdht , char *key , void **pp_value , long *p_value_len );
_WINDLL_EXPORT int DeleteMDHashNode( struct MDHashTree *pmdht , char *key );
_WINDLL_EXPORT int DeleteAllMDHashNode( struct MDHashTree *pmdht );

_WINDLL_EXPORT int TravelMDHashTree( struct MDHashTree *pmdht , char *travel_key_buffer , long travel_key_bufsize , void pfuncTravelProc(char *key,void *value,long value_len,void *pv) , void *pv );
_WINDLL_EXPORT int ReclaimInvalidMDHashNode( struct MDHashTree *pmdht );
_WINDLL_EXPORT struct MDHashNode *GetMDHashRootNode( struct MDHashTree *pmdht );

/* 普通哈希算法 */

typedef BOOL funcFreeHashUnitProc(void *pv) ;

typedef unsigned long funcHashExpressions( char *key ) ;

struct HashArray
{
	unsigned long			prealloc_count ;
	unsigned long			incremental_count ;
	funcHashExpressions		*pfuncHashExpressions ;
	float				inflate_quotiety ;
	
	unsigned long			key_count ;
	unsigned long			total_value_size ;
	
	struct HashUnit
	{
		char			*key ;
		void			*value ;
		long			value_len ;
		funcFreeHashUnitProc	*pfuncFreeHashUnitProc ;
	} *p_units ;
} ;

_WINDLL_EXPORT int InitHashArray( struct HashArray *pha );
_WINDLL_EXPORT int CleanHashArray( struct HashArray *pha );

_WINDLL_EXPORT int PutHashUnit( struct HashArray *pha , char *key , void *value , long value_len , BOOL (*pfuncFreeHashUnitProc)(void *pv) , int mode );

_WINDLL_EXPORT int GetHashUnitPtr( struct HashArray *pha , char *key , void **pp_value , long *p_value_len );
_WINDLL_EXPORT int DeleteHashUnit( struct HashArray *pha , char *key );
_WINDLL_EXPORT int DeleteAllHashUnit( struct HashArray *pha );

_WINDLL_EXPORT int SetHashArrayEnv( struct HashArray *pha , unsigned long prealloc_count , unsigned long increment_count , funcHashExpressions *pfuncHashExpressions );
_WINDLL_EXPORT int SetHashArrayInflateQuotiety( struct HashArray *pha , float inflate_quotiety );

/* 通用哈希容器 */

struct HashContainer
{
	int	algorithm ;
	
	union
	{
		struct MDHashTree	mdht ;
		struct HashArray	ha ;
	} container ;
} ;

_WINDLL_EXPORT int InitHashContainer( struct HashContainer *phc , int algorithm );
_WINDLL_EXPORT int CleanHashContainer( struct HashContainer *phc );

_WINDLL_EXPORT int PutHashItem( struct HashContainer *phc , char *key , void *value , long value_len , BOOL (*pfuncFreeItemProc)(void *pv) , int mode );
_WINDLL_EXPORT int GetHashItemPtr( struct HashContainer *phc , char *key , void **pp_value , long *p_value_len );
_WINDLL_EXPORT int DeleteHashItem( struct HashContainer *phc , char *key );
_WINDLL_EXPORT int DeleteAllHashItem( struct HashContainer *phc );

_WINDLL_EXPORT void *GetHashAlgorithmObject( struct HashContainer *phc , int algorithm );
_WINDLL_EXPORT int TravelHashContainer( struct HashContainer *phc , char *travel_key_buffer , long travel_key_bufsize , void pfuncTravelProc(char *key,void *value,long value_len,void *pv) , void *pv );

#ifdef __cplusplus
}
#endif

#endif
