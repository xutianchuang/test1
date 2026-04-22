#ifndef _BPLC_PHY_H
#define _BPLC_PHY_H


#define MSDU_MAX_BLOCK_SIZE    2096
#define MSDU_BLOCK_NUM   20


typedef struct
{
    BPLC_recv_para rxPara;
    u16 size;
    u8 msdu[MSDU_MAX_BLOCK_SIZE];
}MSDU_BLOCK;


//#define PHY_Printf    printf

MSDU_BLOCK* PHY_MallocMSDU(void);
void PHY_FreeMSDU(MSDU_BLOCK* msdu);
void PHY_Task(void* arg);
void PHY_Init(void);

#endif
