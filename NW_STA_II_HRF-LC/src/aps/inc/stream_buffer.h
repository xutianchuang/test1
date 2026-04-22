#ifndef _STREAM_BUFFER_H_
#define _STREAM_BUFFER_H_

#include "stdint.h"

typedef struct 
{
    uint8_t *read_point;    // 读取数据的指针
    uint8_t *write_point;   // 写入数据的指针
    uint8_t *end_point;     // 缓冲区末尾的指针
    uint8_t *data;          // 指向缓冲区数据存储区的指针
    uint32_t buffer_len;    // 缓冲区长度
} stream_buffer_t;

#ifndef NULL
#define NULL	0
#endif

void stream_buffer_init(stream_buffer_t * buffer, uint8_t *data, uint32_t len);
uint32_t stream_buffer_write(stream_buffer_t * rx_buffer, void *data, uint32_t count);
void stream_buffer_point_shift(stream_buffer_t * rx_buffer, uint8_t **pointer_address, uint32_t length);
void stream_buffer_copy(stream_buffer_t * rx_buffer, uint8_t *dst, uint8_t *src, uint32_t count);
uint32_t stream_buffer_recv_len(stream_buffer_t *rx_buffer);

#endif
