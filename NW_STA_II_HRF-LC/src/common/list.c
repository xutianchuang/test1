#include "common_includes.h"
#include "os.h"
#include <string.h>

#define MAX_LIST_SIZE   256

//PDU内存池
static OS_MEM ListMempool;
static ListNode Listmem[MAX_LIST_SIZE];

//链表模块初始化
void List_Common_Init(void)
{
    OS_ERR err;
    OSMemCreate(&ListMempool,"Block List",Listmem, MAX_LIST_SIZE, sizeof(ListNode), &err);
    if(err != OS_ERR_NONE)
    {
        while(1);
    }
}

//分配一个PDU
static ListNode* MallocListNode(void)
{
    OS_ERR err;
    ListNode *list_ptr = 0;
    list_ptr = (ListNode*)OSMemGet(&ListMempool,&err);
    if(err != OS_ERR_NONE)
        return (void*)0;
    return list_ptr;
}

//释放一个PDU
static void FreeListNode(ListNode* node)
{
    OS_ERR err;
    OSMemPut(&ListMempool,node,&err);
}


//链表初始化
void List_Init(List* l)
{
    memset(l,0,sizeof(List));
}

//末尾插入
bool List_Push_Back(List* l,void* d)
{
    ListNode * node = MallocListNode();
    if(node == NULL)
        return false;
    node->data = d;
    if(l->tail == NULL)
    {
        l->tail = l->head = node;
        l->tail->next = l->tail->pre = NULL;
        l->num++;
    }
    else
    {
        l->tail->next = node;
        node->pre = l->tail;
        l->tail = node;
        l->tail->next = NULL;
        l->num++;
    }
    return true;
}

//前部插入
bool List_Push_Front(List* l,void* d)
{
    ListNode * node = MallocListNode();
    if(node == NULL)
        return false;
    node->data = d;
    if(l->head == NULL)
    {
        l->tail = l->head = node;
        l->tail->next = l->head->pre = NULL;
        l->num++;
    }
    else
    {
        node->next = l->head;
        l->head->pre = node;
        l->head = node;
        l->head->pre = NULL;
        l->num++;
    }
    return true;
}

//返回第一个元素
void* List_Front(List* l)
{
    if(l->head)
        return l->head->data;
    return (void*)0;
}

//返回最后一个元素
void* List_Back(List* l)
{
    if(l->tail)
        return l->tail->data;
    return (void*)0;
}

//删除第一个元素
bool List_Pop_Front(List* l)
{
    if(l->num < 1)
        return false;
    if(l->tail == l->head)
    {
        FreeListNode(l->head);
        l->tail = l->head = NULL;
        l->num--;
    }
    else
    {
        ListNode * node = l->head;
        l->head = l->head->next;
        if(l->head)
            l->head->pre = NULL;
        else
            l->tail = NULL;
        FreeListNode(node);
        l->num--;
    }
    return true;
}

//删除最后一个元素
bool List_Pop_Back(List* l)
{
    if(l->num < 1)
        return false;
    if(l->tail == l->head)
    {
        FreeListNode(l->head);
        l->tail = l->head = NULL;
        l->num--;
    }
    else
    {
        ListNode * node = l->tail;
        l->tail = l->tail->pre;
        if(l->tail)
            l->tail->next = NULL;
        else
            l->head = NULL;
        FreeListNode(node);
        l->num--;
    }
    return true;
}

//删除指定节点
bool List_Del(List* l,ListNode* d)
{
    ListNode * node = 0;
    for(node = l->head;node != NULL;node = node->next)
    {
        if(node == d)
        {
            if(node->pre)
            {
                node->pre->next = node->next;
                if(node->next)
                {
                    node->next->pre = node->pre;
                }
                else    //说明node是尾指针
                {
                    l->tail = node->pre;
                    if(l->tail)
                        l->tail->next = NULL;
                }
            }
            else    //说明node是头指针
            {
                l->head = node->next;
                if(l->head)
                    l->head->pre = NULL;
            }
            if(l->head == NULL || l->tail == NULL)
            {
                l->head = l->tail = NULL;
            }
            FreeListNode(node);
            l->num--;
            return true;
        }
    }
    return false;
}

//删除指定值（全部遍历）
bool List_Del_Data(List* l,void* d)
{
    ListNode * node = 0;
    bool isOk=false;
    for(node = l->head;node != NULL;/*node = node->next*/)
    {
        if(node->data == d)
        {
            ListNode * nextNode = node->next;
            if(node->pre)
            {
                node->pre->next = node->next;
                if(node->next)
                {
                    node->next->pre = node->pre;
                }
                else    //说明node是尾指针
                {
                    l->tail = node->pre;
                }
            }
            else    //说明node是头指针
            {
                l->head = node->next;
				if(l->head)
				  l->head->pre=NULL;
            }
            if( l->tail == node)
            {
                l->tail = node->pre;
				if(l->tail)
				  l->tail->next=NULL;
            }
            FreeListNode(node);
            node = nextNode;
            l->num--;
            isOk=true;
            continue;
        }
        node = node->next;
    }
    return isOk;
}

//清空
bool List_Clear(List* l)
{
    ListNode * node = 0;
    for(node = l->head;node != NULL;/*node = node->next*/)
    {
        ListNode *nextNode = node->next;
        FreeListNode(node);
        node = nextNode;
    }
    l->head = l->tail = NULL;
    l->num = 0;
    return true;
}
