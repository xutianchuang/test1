/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "bspatch.h"

#define WRITE_BLOCK_SIZE    128
static uint8_t old[WRITE_BLOCK_SIZE];
static uint8_t buf_data[WRITE_BLOCK_SIZE];

int32_t offtin(uint8_t *buf)
{
    int32_t y;
    
    y = buf[3] & 0x7F;
    y = y * 256;
    y += buf[2];
    y = y * 256;
    y += buf[1];
    y = y * 256;
    y += buf[0];

    if (buf[3] & 0x80)
    {
        y = -y;
    }
    
    return y;
}



int bspatch(int32_t oldsize, int32_t newsize, struct bspatch_stream *stream)
{
    uint8_t buf[4];
    int32_t oldpos, newpos, len;
    int32_t ctrl[3];
    int32_t i;
    
    oldpos = 0;
    newpos = 0;
    
    while (newpos < newsize)
    {
        /* Read control data */
        for (i = 0; i <= 2; i++)
        {
            if (stream->read(stream, buf, 4))
            {
                return -1;
            }
            
            ctrl[i] = offtin(buf);
        };

        /* Sanity-check */
        if (ctrl[0] < 0 || ctrl[0] > INT_MAX ||
                ctrl[1] < 0 || ctrl[1] > INT_MAX ||
                newpos + ctrl[0] > newsize)
        {
            return -1;
        }
        
        /* Read diff string */
        while (ctrl[0] > 0)
        {
            if (ctrl[0] > WRITE_BLOCK_SIZE)
            {
                len = WRITE_BLOCK_SIZE;
            }
            else
            {
                len = ctrl[0];
            }
            
            if (stream->read(stream, buf_data, len))
            {
                return -1;
            }
			
			if ((oldpos + len > 0) && (oldpos + len <= oldsize))
			{
				if (stream->old_read(stream, oldpos, old, len))
				{
					return -1;
				}
			}
            
            for (i = 0; i < len; i++)
            {
				buf_data[i] += old[i];
            }
            
            stream->write(stream, buf_data, len);
            ctrl[0] -= len;
            oldpos += len;
            newpos += len;
        }
        
        /* Sanity-check */
        if (newpos + ctrl[1] > newsize)
        {
            return -1;
        }
        
        /* Read extra string */
        while (ctrl[1] > 0)
        {
            if (ctrl[1] > WRITE_BLOCK_SIZE)
            {
                len = WRITE_BLOCK_SIZE;
            }
            else
            {
                len = ctrl[1];
            }
            
            if (stream->read(stream, buf_data, len))
            {
                return -1;
            }
            
            stream->write(stream, buf_data, len);
            ctrl[1] -= len;
            newpos += len;
        }
        
        /* Adjust pointers */
        oldpos += ctrl[2];
    };
    
    return 0;
}

