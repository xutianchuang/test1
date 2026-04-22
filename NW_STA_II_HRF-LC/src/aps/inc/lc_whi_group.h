#ifndef __LC_WHI_GROUP_H__
#define __LC_WHI_GROUP_H__

#include "stdint.h"

#define TAG_GROUP_NUM 4
#define SYSCTRL_GROUP_NUM 8

/**
 * @brief 位置分组（TAG分组）
 *
 */
typedef struct __PACKED
{
    uint8_t num;
    uint8_t tag[TAG_GROUP_NUM];
} group_tag_t;

/**
 * @brief 功能分组（SYSCTRL分组）
 *
 */
typedef struct __PACKED
{
    uint8_t num;
    uint16_t addr[SYSCTRL_GROUP_NUM];
} group_sysctrl_t;

bool whi_group_location_set(uint8_t *buf, uint32_t len);
bool whi_group_function_set(uint8_t *buf, uint32_t len);
bool whi_group_sysctrl_addr_check(uint16_t dst_addr);
bool whi_group_tag_check(uint8_t *buf);
bool whi_group_tag_obj_model_check(uint8_t *data, u32 data_len);

#endif
