#ifndef _MAP_H
#define _MAP_H

#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_SUCCESS     1
#define MAP_FAILD       0

typedef void*(*pMallocMapNode)(void);
typedef void(*pFreeMapNode)(void*);

typedef struct rb_root map;
typedef struct rb_node rb_node_t;

//表中的节点单元,目前只支持unsigned long为键值
typedef struct
{
    rb_node_t node;
    unsigned long key;
    void *val;
}map_node;

void SetpMallocMapNode(pMallocMapNode fun);

void SetpFreeMapNode(pFreeMapNode fun);

//根据键值查找节点
map_node *map_t_find(map *root, unsigned long key);

//插入
int map_t_insert(map *root, unsigned long key, void* val);

//返回表的第一个元素
map_node *map_t_first(map *tree);

//返回下一个元素
map_node *map_t_next(rb_node_t *node);

//根据键值删除一个元素
void map_t_erase(map *root,unsigned long key);

//删除一个节点
void map_t_erase_node(map *root,map_node *node);

//清空
void map_t_clear(map *root);
#endif  //_MAP_H
