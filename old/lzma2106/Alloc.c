/* Alloc.c -- Memory allocation functions
2021-07-13 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include <stdio.h>
#include "gsmcu_hal.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdlib.h>

#include "Alloc.h"
//此处申请内存完全是给压缩软件使用 
//使用的是破坏性的申请内存 
//内存管理完全由程序员自己设计 
//申请内存后操作系统将会崩溃 因此解压缩完成后无论成功与否系统必须重启 
//以下为升级时解压缩使用的内存布局 
/*
  ------------------
  |  source
  |   code
  ------------------
  |   stack
  ------------------
  |   my
  |  alloc
  |  mem
 --------------
  |   decompression
  |   code
  --------------------
*/

#pragma segment=".data"

void InitAlloc(void)
{
  u32 stackhead=(u32)__sfb(".data")+DEC_STACK_SIZE;
	stackhead&=0xfffffff8;//8字节对齐
  unsigned char **allocPtr=(unsigned char **)stackhead;//allocPtr是栈帧上面的4个字节  存储解压时动态分配的地址
  allocPtr+=DEC_STACK_SIZE+4;//指向栈的上一个内存空间
  *allocPtr=(unsigned char *)(allocPtr+1);
}
void *MyAlloc(size_t size)
{
  u32 stackhead=(u32)__sfb(".data")+DEC_STACK_SIZE;
	stackhead&=0xfffffff8;//8字节对齐
  unsigned char **allocPtr=(unsigned char **)stackhead;//allocPtr是栈帧上面的4个字节  存储解压时动态分配的地址
  allocPtr+=DEC_STACK_SIZE+4;//指向栈的上一个内存空间
  void * pTemp=(void*)*allocPtr;
  if (size == 0)
    return NULL;
  size=(size+3)/4*4;
  (*allocPtr)+=size;
  return pTemp;
}

void MyFree(void *address)
{
  return ;
}




static void *SzAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p); return MyAlloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p); MyFree(address); }
const ISzAlloc g_Alloc = { SzAlloc, SzFree };

