#include "stream_buffer.h"
#include <stddef.h>
#include <string.h>
// #include "tl_common.h"

// _attribute_ram_code_ void stream_memcpy(register uint8_t * dest, register uint8_t * src, int len) 
void stream_memcpy(register uint8_t * dest, register uint8_t * src, int len) 
{
	while (len--)
        *dest++ = *src++;
}

// _attribute_ram_code_ uint32_t stream_buffer_write(stream_buffer_t * rx_buffer, void *data, uint32_t count)
uint32_t stream_buffer_write(stream_buffer_t * rx_buffer, void *data, uint32_t count)
{
    uint32_t surplus = 0;
    if (rx_buffer == NULL)
    {
        return 0;
    }
    /* (data)----(r)----(w)----(end) */
    if (rx_buffer->write_point >= rx_buffer->read_point)
    {
        uint32_t w2end = rx_buffer->end_point - rx_buffer->write_point;
        surplus = rx_buffer->buffer_len - (rx_buffer->write_point - rx_buffer->read_point);
        count = count > surplus ? surplus : count;
        if (count >= w2end)
        {
            stream_memcpy(rx_buffer->write_point, data, w2end);
            rx_buffer->write_point = rx_buffer->data;

            stream_memcpy(rx_buffer->write_point, (uint8_t *)data + w2end, (count - w2end));
            rx_buffer->write_point += (count - w2end);
        }
        else
        {
            stream_memcpy(rx_buffer->write_point, data, count);
            rx_buffer->write_point += count;
        }
    }
    else    /* (data)----(w)----(r)----(end) */
    {
        surplus = rx_buffer->read_point - rx_buffer->write_point;
        count = count > surplus ? surplus : count;
        stream_memcpy(rx_buffer->write_point, data, count);
        rx_buffer->write_point += count;
    }
    return count;
}

/* increases buffer pointer by one and circle around if necessary */
void stream_buffer_point_shift(stream_buffer_t * rx_buffer, uint8_t **pointer_address, uint32_t length)
{
    uint8_t *pointer = *pointer_address + length;

    if (rx_buffer->write_point >= rx_buffer->read_point)
    {
        if (pointer >= rx_buffer->write_point)
        {
            *pointer_address = rx_buffer->write_point;
        }
        else
        {
            *pointer_address = pointer;
        }
    }
    else
    {
        if (pointer >= rx_buffer->end_point)
        {
            *pointer_address = rx_buffer->data;
            pointer = pointer - rx_buffer->end_point + rx_buffer->data;

            if (pointer >= rx_buffer->write_point)
            {
                *pointer_address = rx_buffer->write_point;
            }
            else
            {
                *pointer_address = pointer;
            }
        }
        else
        {
            *pointer_address = pointer;
        }
    }
}

/* copy data from receive buffer */
void stream_buffer_copy(stream_buffer_t * rx_buffer, uint8_t *dst, uint8_t *src, uint32_t count)
{
    uint8_t *pointer = NULL;

    pointer = src + count;
    if (pointer >= rx_buffer->end_point)
    {
        uint32_t offset = 0;
        offset = rx_buffer->end_point - src;
        stream_memcpy(dst, src, offset);
        stream_memcpy(dst + offset,  rx_buffer->data, pointer - rx_buffer->end_point);
    }
    else
    {
        stream_memcpy(dst, src, count);
    }
}

/* Length of data received */
uint32_t stream_buffer_recv_len(stream_buffer_t *rx_buffer)
{
    if (rx_buffer == NULL)
    {
        return 0;
    }
    if (rx_buffer->write_point >= rx_buffer->read_point)
    {
        return (rx_buffer->write_point - rx_buffer->read_point);
    }
    else
    {
        return (rx_buffer->buffer_len - (rx_buffer->read_point - rx_buffer->write_point));
    }
}


void stream_buffer_init(stream_buffer_t * buffer, uint8_t *data, uint32_t len)
{
    memset(buffer, 0, sizeof(stream_buffer_t));

    buffer->data = data;
    buffer->buffer_len = len;

    buffer->read_point = buffer->data;
    buffer->write_point = buffer->data;
    buffer->end_point = buffer->data + buffer->buffer_len; 
}

