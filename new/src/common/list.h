#ifndef _LIST_H_
#define _LIST_H_

typedef struct _ListNode
{
    void* data;
    struct _ListNode *pre;
    struct _ListNode *next;
}ListNode;

typedef struct
{
    ListNode* head;
    ListNode* tail;
    int num;
}List;

//链表模块初始化
void List_Common_Init(void);

//链表初始化
void List_Init(List* l);

//末尾插入
bool List_Push_Back(List* l,void* d);

//前部插入
bool List_Push_Front(List* l,void* d);

//返回第一个元素
void* List_Front(List* l);

//返回最后一个元素
void* List_Back(List* l);

//删除第一个元素
bool List_Pop_Front(List* l);

//删除最后一个元素
bool List_Pop_Back(List* l);

//删除指定节点
bool List_Del(List* l,ListNode* d);

//删除指定值（全部遍历）
bool List_Del_Data(List* l,void* d);

//清空
bool List_Clear(List* l);
#endif
