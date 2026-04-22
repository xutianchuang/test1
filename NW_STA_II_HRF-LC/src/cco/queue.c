
/*	queue.c

	Implementation of a FIFO queue abstract data type.


*/


#include "system_inc.h"
#include "alert.h"
#include "queue.h"

queue * define_new_queue(queue *pNewQueue, U16 queue_size)
{
	if(pNewQueue == NULL)
		return NULL;
	
	pNewQueue->first = 0;
	//pNewQueue->last = queue_size-1;	/* alan check - 貌似这里应该是: END_TX_QUEUE_SIZE  */
	pNewQueue->last = 0;	
	pNewQueue->count = 0;
	pNewQueue->maxcount = queue_size;		// 50

	return pNewQueue;
}





U16 enqueue(queue *q, HANDLE x)
{
	
	if(q)
	{
		if (q->count >= q->maxcount)
		{			
			//Alert(ALERT_FULL_QUEUE, ALERT_NO_ACTION, __FILE__, __LINE__);
            return ERROR;
		}
		else 
		{		
            CPU_SR_ALLOC();
            OS_CRITICAL_ENTER(); 
			q->q[ q->last ] = x;    
			q->count = q->count + 1;
			q->last = (q->last+1) % (q->maxcount);		
            OS_CRITICAL_EXIT();
		}
		
	}
    return OK;
}

HANDLE dequeue(queue *q)
{
	
    HANDLE x = NULL;
	if(q)
	{
	
		if (q->count <= 0)
		{
			//printf("Warning: empty queue dequeue.\n");
			//alert();
			//return NULL;
		}
		else 
		{
	
		    CPU_SR_ALLOC();
            OS_CRITICAL_ENTER();
			x = q->q[ q->first ];			
			q->first = (q->first+1) % (q->maxcount);
			q->count = q->count - 1;
            OS_CRITICAL_EXIT();
            
		}
	}
    return(x);
}

int is_queue_empty(queue *q)
{
    if (q->count <= 0) 
		return (TRUE);
    else 
		return (FALSE);
}

USHORT get_queue_cnt(queue *q)
{
	if(q)
	{
		if ( (q->count > q->maxcount) )
		{
			//Alert(ALERT_FULL_QUEUE, ALERT_NO_ACTION, __FILE__, __LINE__);
		}
		else 
		{
			return (q->count);
		}
	}

	return 0xFFFF;
}


