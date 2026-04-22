#include "common_includes.h"

static pMallocMapNode MallocMapNode = 0;
static pFreeMapNode FreeMapNode = 0;

void SetpMallocMapNode(pMallocMapNode fun)
{
    MallocMapNode = fun;
}

void SetpFreeMapNode(pFreeMapNode fun)
{
    FreeMapNode = fun;
}

map_node *map_t_find(map *root, unsigned long key) {
   rb_node_t *node = root->rb_node; 
   while (node) {

       const rb_node_t * __mptr = node;
       map_node *data = (map_node*)((char *)__mptr - offsetof(map_node,node));
       
        if(key < data->key)
            node = node->rb_left;
        else if(key > data->key)
            node = node->rb_right;
        else
            return data;
   }
   return NULL;
}

int map_t_insert(map *root, unsigned long key, void* val) 
{
    map_node *data = (map_node*)MallocMapNode();
	if(data == NULL)
		return MAP_FAILD;
    data->key = key;
    data->val = val;
    rb_node_t **new_node = &(root->rb_node), *parent = NULL;
    while (*new_node) {
        //map_node *this_node = container_of(*new_node, map_node, node);
        
        const rb_node_t * __mptr = *new_node;
        map_node *this_node = (map_node*)((char *)__mptr - offsetof(map_node,node));
        
        parent = *new_node;

        if (key < this_node->key) 
            new_node = &((*new_node)->rb_left);
        else if (key > this_node->key)
            new_node = &((*new_node)->rb_right);
        else 
        {
            FreeMapNode(data);
            return MAP_FAILD;
        }
    }

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);

    return MAP_SUCCESS;
}

map_node *map_t_first(map *tree) {
    rb_node_t *node = rb_first(tree);
    //return (rb_entry(node, map_node, node));
    const rb_node_t * __mptr = node;
    return (map_node*)((char *)__mptr - offsetof(map_node,node));
    
}

map_node *map_t_next(rb_node_t *node) {
    rb_node_t *next =  rb_next(node);
    //return rb_entry(next, map_node, node);
    const rb_node_t * __mptr = next;
    return (map_node*)((char *)__mptr - offsetof(map_node,node));
}

void map_t_erase(map *root,unsigned long key)
{
    map_node * node = map_t_find(root,key);
    if(node != NULL)
    {
        rb_erase(&node->node,root);
        FreeMapNode(node);
    }
}

void map_t_erase_node(map *root,map_node *node)
{
    rb_erase(&node->node,root);
    FreeMapNode(node);
}

//Çå¿Õ
void map_t_clear(map *root)
{
    map_node *node = map_t_first(root);
    for(;node != NULL;)
    {
        map_node * nextnode = map_t_next(&node->node);
        map_t_erase_node(root,node);
        node = nextnode;
    }
}
