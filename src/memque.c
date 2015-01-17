#include "memque.h"

int CreateMemoryQueue( struct MemoryQueue **pp_queue , unsigned long queue_totalsize , long max_message_count , long max_message_size )
{
	if( pp_queue == NULL || queue_totalsize < sizeof(struct MemoryQueue) + sizeof(struct QueueBlock) )
		return MEMQUEUE_ERROR_PARAMETER;
	
	(*pp_queue) = (struct MemoryQueue *)malloc( queue_totalsize ) ;
	if( (*pp_queue) == NULL )
		return MEMQUEUE_ERROR_ALLOC;
	
	InitMemoryQueue( (*pp_queue) , queue_totalsize , max_message_count , max_message_size );
	
	return 0;
}

int DestroyMemoryQueue( struct MemoryQueue **pp_queue )
{
	if( pp_queue == NULL || (*pp_queue) == NULL )
		return MEMQUEUE_ERROR_PARAMETER;
	
	free( (*pp_queue) );
	
	return 0;
}

int InitMemoryQueue( struct MemoryQueue *p_queue , unsigned long queue_totalsize , long max_message_count , long max_message_size )
{
	memset( (char*)p_queue , 0x00 , queue_totalsize );
	
	p_queue->queue_totalsize = queue_totalsize ;
	p_queue->max_message_count = max_message_count ;
	p_queue->max_message_size = max_message_size ;
	
	p_queue->used_totalsize = sizeof(struct MemoryQueue) ;
	
	return 0;
}

int CleanMemoryQueue( struct MemoryQueue *p_queue )
{
	return InitMemoryQueue( p_queue , p_queue->queue_totalsize , p_queue->max_message_count , p_queue->max_message_size );
}

static int AllocQueueBlock( struct MemoryQueue *p_queue , unsigned long block_size , struct QueueBlock **pp_block )
{
	if( p_queue == NULL || block_size < 0 )
		return MEMQUEUE_ERROR_PARAMETER;
	if( p_queue->used_totalsize + block_size > p_queue->queue_totalsize )
		return MEMQUEUE_ERROR_QUEUE_OVERFLOW;
	if( p_queue->max_message_count != -1 && p_queue->block_count + 1 > p_queue->max_message_count )
		return MEMQUEUE_ERROR_TOOMANY_BLOCK;
	if( p_queue->max_message_size != -1 && block_size > p_queue->max_message_size )
		return MEMQUEUE_ERROR_TOOBIG_BLOCK;
	
	if( p_queue->first_addr_links == NULL )
	{
		if( sizeof(struct MemoryQueue) + sizeof(struct QueueBlock) + block_size <= p_queue->queue_totalsize )
		{
			/*
				|H|                  |
				|H|ADD|              |
			*/
			struct QueueBlock	*p_block_add = NULL ;
			
			p_block_add = (struct QueueBlock *)( (char*)p_queue + sizeof(struct MemoryQueue) ) ;
			
			p_block_add->block_size = block_size ;
			p_block_add->prev_addr = NULL ;
			p_block_add->next_addr = NULL ;
			p_block_add->prev_order = NULL ;
			p_block_add->next_order = NULL ;
			
			p_queue->first_addr_links = p_block_add ;
			p_queue->last_addr_links = p_block_add ;
			p_queue->first_order_links = p_block_add ;
			p_queue->last_order_links = p_block_add ;
			
			p_queue->block_count++;
			p_queue->used_totalsize += sizeof(struct QueueBlock) + block_size ;
			
			if( pp_block )
				(*pp_block) = p_block_add ;
			return 0;
		}
	}
	else
	{
		struct QueueBlock	*p_block_travel = NULL ;
		unsigned long		block_count ;
		
		for( p_block_travel = p_queue->last_addr_links , block_count = 0 ; block_count < p_queue->block_count ; p_block_travel = (p_block_travel->next_addr!=NULL?p_block_travel->next_addr:p_queue->first_addr_links) , block_count++ )
		{
			if(	p_block_travel->next_addr == NULL
				&&
				(char*)p_block_travel + sizeof(struct QueueBlock) + p_block_travel->block_size + sizeof(struct QueueBlock) + block_size <= (char*)p_queue + p_queue->queue_totalsize )
			{
				/*
					|H|                  |
					|H|BLOCK|ADD|        |
				*/
				struct QueueBlock	*p_block_add = NULL ;
				
				p_block_add = (struct QueueBlock *)( (char*)p_block_travel + sizeof(struct QueueBlock) + p_block_travel->block_size ) ;
				
				p_block_add->block_size = block_size ;
				p_block_add->prev_addr = p_block_travel ;
				p_block_add->next_addr = NULL ;
				p_block_add->prev_order = p_queue->last_order_links ;
				p_block_add->next_order = NULL ;
				
				p_queue->last_addr_links->next_addr = p_block_add ;
				p_queue->last_addr_links = p_block_add ;
				
				p_queue->last_order_links->next_order = p_block_add ;
				p_queue->last_order_links = p_block_add ;
				
				p_queue->block_count++;
				p_queue->used_totalsize += sizeof(struct QueueBlock) + block_size ;
				
				if( pp_block )
					(*pp_block) = p_block_add ;
				return 0;
			}
			else if(	p_block_travel->prev_addr == NULL
					&&
					(char*)p_queue + sizeof(struct MemoryQueue) + sizeof(struct QueueBlock) + block_size <= (char*)(p_block_travel) )
			{
				/*
					|H|                  |
					|H|ADD|  |BLOCK|     |
				*/
				struct QueueBlock	*p_block_add = NULL ;
				
				p_block_add = (struct QueueBlock *)( (char*)p_queue + sizeof(struct MemoryQueue) ) ;
				
				p_block_add->block_size = block_size ;
				p_block_add->prev_addr = NULL ;
				p_block_add->next_addr = p_queue->first_addr_links ;
				p_block_add->prev_order = p_queue->last_order_links ;
				p_block_add->next_order = NULL ;
				
				p_queue->first_addr_links->prev_addr = p_block_add ;
				p_queue->first_addr_links = p_block_add ;
				
				p_queue->last_order_links->next_order = p_block_add ;
				p_queue->last_order_links = p_block_add ;
				
				p_queue->block_count++;
				p_queue->used_totalsize += sizeof(struct QueueBlock) + block_size ;
				
				if( pp_block )
					(*pp_block) = p_block_add ;
				return 0;
			}
			else if( (char*)p_block_travel + sizeof(struct QueueBlock) + p_block_travel->block_size + sizeof(struct QueueBlock) + block_size <= (char*)(p_block_travel->next_addr) )
			{
				/*
					|H|                      |
					|H|BLOCK|ADD|   |BLOCK|  |
				*/
				struct QueueBlock	*p_block_add = NULL ;
				
				p_block_add = (struct QueueBlock *)( (char*)p_block_travel + sizeof(struct QueueBlock) + p_block_travel->block_size ) ;
				
				p_block_add->block_size = block_size ;
				p_block_add->prev_addr = p_block_travel ;
				p_block_add->next_addr = p_block_travel->next_addr ;
				p_block_add->prev_order = p_queue->last_order_links ;
				p_block_add->next_order = NULL ;
				
				p_block_travel->next_addr = p_block_add ;
				p_block_travel->next_addr->prev_addr = p_block_add ;
				
				p_queue->last_order_links->next_order = p_block_add ;
				p_queue->last_order_links = p_block_add ;
				
				p_queue->block_count++;
				p_queue->used_totalsize += sizeof(struct QueueBlock) + block_size ;
				
				if( pp_block )
					(*pp_block) = p_block_add ;
				return 0;
			}
		}
	}
	
	return MEMQUEUE_ERROR_NOT_ENOUGH_SPACE;
}

int AddQueueBlock( struct MemoryQueue *p_queue , char *block_data , unsigned long block_size , struct QueueBlock **pp_block )
{
	struct QueueBlock	*p_block = NULL ;
	int			nret ;
	
	if( p_queue == NULL || block_size < 0 )
		return MEMQUEUE_ERROR_PARAMETER;
	
	nret = AllocQueueBlock( p_queue , block_size , & p_block ) ;
	if( nret )
		return nret;
	
	memcpy( (char*)p_block + sizeof(struct QueueBlock) , block_data , block_size );
	
	if( pp_block && block_data )
		(*pp_block) = p_block ;
	
	return 0;
}

int RemoveQueueBlock( struct MemoryQueue *p_queue , struct QueueBlock *p_block )
{
	struct QueueBlock	*p_block_prev_addr = NULL ;
	struct QueueBlock	*p_block_next_addr = NULL ;
	struct QueueBlock	*p_block_prev_order = NULL ;
	struct QueueBlock	*p_block_next_order = NULL ;
	
	if( p_queue == NULL || p_block == NULL )
		return MEMQUEUE_ERROR_PARAMETER;
	
	p_queue->block_count--;
	p_queue->used_totalsize -= sizeof(struct QueueBlock) + p_block->block_size ;
	
	if( p_queue->first_addr_links == p_block )
		p_queue->first_addr_links = p_block->next_addr ;
	if( p_queue->last_addr_links == p_block )
		p_queue->last_addr_links = p_block->prev_addr ;
	
	if( p_queue->first_order_links == p_block )
		p_queue->first_order_links = p_block->next_order ;
	if( p_queue->last_order_links == p_block )
		p_queue->last_order_links = p_block->prev_order ;
	
	p_block_prev_addr = p_block->prev_addr ;
	p_block_next_addr = p_block->next_addr ;
	if( p_block_prev_addr )
		p_block_prev_addr->next_addr = p_block_next_addr ;
	if( p_block_next_addr )
		p_block_next_addr->prev_addr = p_block_prev_addr ;
	
	p_block_prev_order = p_block->prev_order ;
	p_block_next_order = p_block->next_order ;
	if( p_block_prev_order )
		p_block_prev_order->next_order = p_block_next_order ;
	if( p_block_next_order )
		p_block_next_order->prev_order = p_block_prev_order ;
	
	return 0;
}

int TravelQueueBlockByAddr( struct MemoryQueue *p_queue , struct QueueBlock **pp_block )
{
	if( p_queue == NULL || pp_block == NULL )
		return MEMQUEUE_ERROR_PARAMETER;
	
	if( (*pp_block) == NULL )
		(*pp_block) = p_queue->first_addr_links ;
	else
		(*pp_block) = (*pp_block)->next_addr ;
	
	if( (*pp_block) )
		return 0;
	else
		return MEMQUEUE_WARN_NO_BLOCK;
}

int TravelQueueBlockByOrder( struct MemoryQueue *p_queue , struct QueueBlock **pp_block )
{
	if( p_queue == NULL || pp_block == NULL )
		return MEMQUEUE_ERROR_PARAMETER;
	
	if( (*pp_block) == NULL )
		(*pp_block) = p_queue->first_order_links ;
	else
		(*pp_block) = (*pp_block)->next_order ;
	
	if( (*pp_block) )
		return 0;
	else
		return MEMQUEUE_WARN_NO_BLOCK;
}
