#include "common_includes.h"
#include "os.h"

//map?¨²¡ä??¨¢113?¨º??¡¥
#define MAX_MAP_NODE_SIZE    512
//static OS_MEM MapMempool;
//static map_node MapMem[MAX_MAP_NODE_SIZE];

//static void* MallocMap(void)
//{
//	OS_ERR err;
//    map_node *mem_ptr = 0;
//    mem_ptr = (map_node*)OSMemGet(&MapMempool,&err);
//    if(err != OS_ERR_NONE)
//        return (void*)0;
//    return mem_ptr;
//}
//
//static void FreeMap(void* node)
//{
//	OS_ERR err;
//    OSMemPut(&MapMempool,node,&err);
//}

//1?12¡Á¨¦?t?¡ê?¨¦3?¨º??¡¥
void CommonInit(void)
{
    //?¨²¡ä?3?3?¨º??¡¥
    //OS_ERR err;
 //   OSMemCreate(&MapMempool,"MapNode Memory",MapMem, MAX_MAP_NODE_SIZE, sizeof(map_node), &err);
//    if(err != OS_ERR_NONE)
//    {
//        while(1);
//    }
    //SetpMallocMapNode(MallocMap);
    //SetpFreeMapNode(FreeMap);
    
    //
    List_Common_Init();
}