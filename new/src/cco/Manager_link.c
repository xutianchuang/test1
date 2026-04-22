/*
*******************************************************************************
** Include files
*******************************************************************************
*/

#include "system_inc.h"
#include "alert.h"

GLOBAL_CFG_PARA     g_PlmConfigPara;
GLOBAL_STATUS_PARA  g_PlmStatusPara;


OS_RES        g_os_res;
P_OS_RES      g_pOsRes = NULL;

typedef struct
{
    OS_MEM*         p_os_mem;
    void*           p_mem;
    unsigned short  blk_size;
    unsigned char   blk_count;
}MEM_POOL_INFO, *P_MEM_POOL_INFO;

#define MEM_POOL_COUNT   7

static OS_MEM os_mem_pool_array[MEM_POOL_COUNT];

static unsigned char mem_0[60][sizeof(TMSG)];
#if defined(PROTOCOL_GD_2016)
//#if 0
static unsigned char mem_1[READ_TASK_SIZE*3/2][sizeof(MSG_HEADER)+PLC_BUFFER_UART_SIZE];     //14
#else
static unsigned char mem_1[100][sizeof(MSG_HEADER)+PLC_BUFFER_UART_SIZE];     //14
#endif
static unsigned char mem_2[6][1024];                                         //6
static unsigned char mem_3[15][sizeof(MSG_HEADER)+MSA_MESSAGAE_MAX_SIZE];        //MSG_HEADER 与 LMP_APP_INFO，取大的那个
//static unsigned char mem_4[18][2096+2];        //MAX_MPDU_FRAME_LEN      PDU_BLOCK
static unsigned char mem_4[18][sizeof(PDU_BLOCK)+2];        //MAX_MPDU_FRAME_LEN      PDU_BLOCK
static unsigned char mem_5[1][sizeof(PCO_BITMAP_SUBSTA)];
static unsigned char mem_6[1][sizeof(RELAY_PATH)*MAX_NOT_CENTER_BEACON_LIMIT];

static MEM_POOL_INFO mem_pool_info_array[MEM_POOL_COUNT] = 
    {
        {&os_mem_pool_array[0], &mem_0[0], sizeof(mem_0[0]), countof(mem_0)}, 
        {&os_mem_pool_array[1], &mem_1[0], sizeof(mem_1[0]), countof(mem_1)},
        {&os_mem_pool_array[2], &mem_2[0], sizeof(mem_2[0]), countof(mem_2)},
        {&os_mem_pool_array[3], &mem_3[0], sizeof(mem_3[0]), countof(mem_3)},
        {&os_mem_pool_array[4], &mem_4[0], sizeof(mem_4[0]), countof(mem_4)},
        {&os_mem_pool_array[5], &mem_5[0], sizeof(mem_5[0]), countof(mem_5)},
        {&os_mem_pool_array[6], &mem_6[0], sizeof(mem_6[0]), countof(mem_6)}
    };


void defqueue(FAST_QUEUE *fpq, U32 *storage, U16 size, U16 sema )
{
	fpq->storage = storage;
	fpq->get = 0;
	fpq->put = 0;
	fpq->size = size;
//	fpq->lock = 0;
	fpq->sema = sema;
}

PVOID sema_dequeue(FAST_QUEUE *fpq)
{
	U16 get;
	U32 p = 0;

	get = fpq->get;

	if (fpq->put != get)
	{
		p = fpq->storage[get];

        CPU_SR_ALLOC();
        OS_CRITICAL_ENTER();
		if ( ++get == fpq->size)
			get = 0;
		fpq->get = get;
        OS_CRITICAL_EXIT();
	}

	return (void *)p;
}

#if 0
U32 sema_enqueue(FAST_QUEUE *fpq, void *p, OS_EVENT  * sema)
{
	U16 put;

	U32 rc = 0;

	put = fpq->put;

    OS_ENTER_CRITICAL();
	if (++put >= fpq->size)
		put = 0;
    OS_EXIT_CRITICAL();

	if (put != fpq->get)
	{
		rc = 1;
        OS_ENTER_CRITICAL();
		fpq->storage[fpq->put] = (U32)p;
		fpq->put = put;
        OS_EXIT_CRITICAL();
	}

	// 激活事件量
	OSSemPost(sema);
 
	return rc;
}
#endif

/*******************************************************

函数说明:  从pMsgTxPool[i]  中申请一帧空闲buffer 

*******************************************************/
P_MSG_INFO alloc_send_buffer(const char* func_name,MSG_TTYPE type)
{
    P_MSG_INFO pMsg = NULL;

    if(MSG_SHORT==type)
    {
        pMsg = (P_MSG_INFO)alloc_buffer(func_name,sizeof(MSG_HEADER)+PLC_BUFFER_UART_SIZE);
        if(NULL != pMsg)
            memset_special(pMsg->msg_buffer, 0xff, PLC_BUFFER_UART_SIZE);
    }
    else
    {
        pMsg = (P_MSG_INFO)alloc_buffer(func_name,sizeof(MSG_HEADER)+MSA_MESSAGAE_MAX_SIZE);
        if(NULL != pMsg)
            memset_special(pMsg->msg_buffer, 0xff, MSA_MESSAGAE_MAX_SIZE);
    }

    return pMsg;
}

P_MSG_INFO alloc_send_buffer_by_len(const char* func_name,USHORT len)
{
    len = sizeof(MSG_HEADER) + len;

    P_MSG_INFO pMsg = (P_MSG_INFO)alloc_buffer(func_name,len);

    return pMsg;
}
    
/*******************************************************

函数说明:  从pMsgTxPool[i]  中释放一帧数据buffer (   *pmsg
) 

*******************************************************/

unsigned char free_send_buffer(const char* func_name,pvoid pmsg)
{
    free_buffer(func_name,pmsg);
	return 0;
}

P_TMSG alloc_msg(const char* func_name)
{
    P_TMSG pMsg = (P_TMSG)alloc_buffer(func_name,sizeof(TMSG));
    if(NULL != pMsg)
    {
        pMsg->wParam = 0;
        pMsg->lParam = 0;
    }
    return pMsg;
}

void free_msg(const char* func_name,P_TMSG pMsg)
{
    free_buffer(func_name,pMsg);
}

//占用最大内存数是 MAX_MAC_FRAME_LEN  2078 字节
PBYTE alloc_buffer(const char* func_name,USHORT len)
{
    OS_ERR err;
    PBYTE p_alloc = NULL;
	unsigned char i;
    for(i=0; i<MEM_POOL_COUNT; i++)
    {
        if(mem_pool_info_array[i].blk_size >= len)
        {
            p_alloc = (PBYTE)OSMemGet(mem_pool_info_array[i].p_os_mem, &err);
            if(NULL != p_alloc)
                break;
        }
    }
	if(NULL != p_alloc)
	{
		u32 num = (u32)p_alloc - (u32)(mem_pool_info_array[i].p_mem);
		num /= mem_pool_info_array[i].blk_size;
		debug_str(DEBUG_LOG_ALLOC,"%s alloc new buffer [%d][%d]\r\n",func_name,i,num);

	}
	else
	{
		debug_str(DEBUG_LOG_ERR,"%s no alloc buffer, len %d\r\n", func_name, len);
	}
	return p_alloc;
}

void free_buffer(const char* func_name,PVOID pBuffer)
{
    if(NULL == pBuffer)
	{
        return;
	}

    OS_ERR err;
    OS_MEM* p_os_mem = NULL;
	unsigned short i = 0, j;

    for(i=0; i<MEM_POOL_COUNT; i++)
    {
        P_MEM_POOL_INFO pPool = &mem_pool_info_array[i];
        PBYTE pmem = (PBYTE)pPool->p_mem;

        for(j=0; j<pPool->blk_count; j++)
        {
            if(pmem==pBuffer)
            {
                p_os_mem = pPool->p_os_mem;
                break;
            }
            pmem += pPool->blk_size;
        }

        if(NULL != p_os_mem)
            break;
    }

    if(NULL != p_os_mem)
	{
        OSMemPut(p_os_mem, pBuffer, &err);
		debug_str(DEBUG_LOG_ALLOC,"%s free buffer [%d][%d]\r\n",func_name,i,j);
	}
	else
	{
      debug_str(DEBUG_LOG_ERR,"%s free buffer not find pBuffer:%p\r\n",func_name,pBuffer);
	}
}

void ManagerLinkInit()
{
    OS_ERR err;
	unsigned short i;

    memset_special(&g_PlmStatusPara, 0, sizeof(g_PlmStatusPara));

    g_pOsRes  = &g_os_res;
    //g_pOsRes = malloc(sizeof(OS_RES));

    for(i=0; i<countof(mem_pool_info_array); i++)
    {
        P_MEM_POOL_INFO pPool = &mem_pool_info_array[i];
        OSMemCreate(pPool->p_os_mem, NULL, pPool->p_mem, pPool->blk_count, pPool->blk_size, &err);
    }

//    OSQCreate(&g_pOsRes->mac_frame_q, "mac_frame_q", 8, &err);
//    OSQCreate(&g_pOsRes->plm_frame_q, "plm_frame_q", 5, &err);
    OSQCreate(&g_pOsRes->q_up_rx_plc, "q_up_rx_plc", 1, &err);

#if defined(PROTOCOL_GD_2016)
    OSQCreate(&g_pOsRes->read_meter_task_q, "read_meter_task_q", READ_TASK_SIZE, &err);
#endif

    OSSemCreate(&g_pOsRes->savedata_mbox, "savedata_mbox", 0, &err);
    OSSemCreate(&g_pOsRes->sem_wait_send_complete, "sem_wait_send_complete", 0, &err);
    OSSemCreate(&g_pOsRes->sem_wait_debug_send_done, "debug tx done sem", 0, &err);
}


