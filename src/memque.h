#ifndef _H_MEMQUEUE_
#define _H_MEMQUEUE_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMQUEUE_ERROR_PARAMETER	-11
#define MEMQUEUE_ERROR_ALLOC		-21
#define MEMQUEUE_ERROR_NOT_ENOUGH_SPACE	-31
#define MEMQUEUE_WARN_NO_BLOCK		91
#define MEMQUEUE_ERROR_QUEUE_OVERFLOW	-41			
#define MEMQUEUE_ERROR_TOOMANY_BLOCK	-42
#define MEMQUEUE_ERROR_TOOBIG_BLOCK	-43

struct QueueBlock
{
	unsigned long		block_size ;
	
	struct QueueBlock	*prev_addr ;
	struct QueueBlock	*next_addr ;
	struct QueueBlock	*prev_order ;
	struct QueueBlock	*next_order ;
} ;

struct MemoryQueue
{
	unsigned long		queue_totalsize ;
	unsigned long		max_message_count ;
	unsigned long		max_message_size ;
	
	unsigned long		block_count ;
	unsigned long		used_totalsize ;
	
	struct QueueBlock	*first_addr_links ;
	struct QueueBlock	*last_addr_links ;
	struct QueueBlock	*first_order_links ;
	struct QueueBlock	*last_order_links ;
	
	char			reserve[ 1024 + 1 ] ;
} ;

int CreateMemoryQueue( struct MemoryQueue **pp_queue , unsigned long queue_totalsize , long max_message_count , long max_message_size );
int DestroyMemoryQueue( struct MemoryQueue **pp_queue );

int InitMemoryQueue( struct MemoryQueue *p_queue , unsigned long queue_totalsize , long max_message_count , long max_message_size );
int CleanMemoryQueue( struct MemoryQueue *p_queue );

int AddQueueBlock( struct MemoryQueue *p_queue , char *block_data , unsigned long block_size , struct QueueBlock **pp_block );
int RemoveQueueBlock( struct MemoryQueue *p_queue , struct QueueBlock *p_block );

int TravelQueueBlockByAddr( struct MemoryQueue *p_queue , struct QueueBlock **pp_block );
int TravelQueueBlockByOrder( struct MemoryQueue *p_queue , struct QueueBlock **pp_block );

#endif
