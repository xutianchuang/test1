//#include <ucos_ii.h>
//#include "../hal/macrodriver.h"
#include "type_define.h"
#include "system_inc.h"
#include "manager_link.h"
#include "alert.h"
#include "module_def.h"

unsigned char RT_init(HANDLE h);
unsigned char RT_proc(HANDLE h);

extern unsigned short Task_init(HANDLE h);
extern unsigned short PLM_init(HANDLE h);
extern unsigned short PLC_init(HANDLE h);
extern unsigned short PLC_proc(HANDLE h);



