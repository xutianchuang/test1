#include <stdlib.h>
#include <string.h>
#include "lc_all.h"

bool whi_group_location_set(uint8_t *buf, uint32_t len)
{
   
    return true;
}

bool whi_group_function_set(uint8_t *buf, uint32_t len)
{
    
    return true;
}

bool whi_group_sysctrl_addr_check(uint16_t dst_addr)
{
    if (dst_addr == 0x0000)
        return true;
    if (dst_addr == 0xFFFF)
        return true;
    if (dst_addr == 0x4000)
        return true;
   
    return false;
}



bool whi_group_tag_obj_model_check(uint8_t *data, u32 data_len)
{
    bool flag;
    uint32_t obj_idx;
    uint16_t obj_len;
    object_model_interface_t *p_obj;

    obj_idx = 0;
    flag = true;
    while (data_len >= 4)
    {
        p_obj = (void *)&data[obj_idx];
        obj_len = OBJECT_MODE_HEAD_START_SIZE + p_obj->data_len;
        if (data_len < obj_len)
        {
            break;
        }
        else
        {
            data_len -= obj_len;
            obj_idx += obj_len;
        }

        if (p_obj->SIID == SIID_TAG_OPERATE_OBJ)
        {
            flag = false;
            break;
        }
    }
    return flag;
}