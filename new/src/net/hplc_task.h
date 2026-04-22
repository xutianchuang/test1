#ifndef _HPLC_TASK_H_
#define _HPLC_TASK_H_


//plc?-?¨¦?¨©¨©¨½
void HPLC_Task(void* arg);

//?¨©¨©¨½3?????

void Reset_CallBack(u32 arg);
void Reset_Proc(void);
extern const u8 USE_OLD_PHASE;
extern const u8 USE_BAND1_TESE_MODE;
extern const u8 HRF_LinePower;
extern const u8 HRF_TestChlPower[];
extern const u8 HRF_PowerOffPower;

#endif
