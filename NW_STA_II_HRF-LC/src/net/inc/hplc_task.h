#ifndef _HPLC_TASK_H_
#define _HPLC_TASK_H_


//plc协议任务
void HPLC_Task(void* arg);

//任务初始化
void HPLC_Task_Init(void);

//接收物理块回调函数
void ReceivePhyCallback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para);

//接收FC回调函数
void ReceiveCtrlCallback(uint8_t *data, BPLC_recv_para *para, bool_t status);
//冻结曲线任务
void HPLC_GraphTask(void *arg);
u8 GetGraphCs(Graph_Data_s *pData);
//二采校时任务
void HPLC_II_TimingTask(void *arg);

#ifndef II_STA
#ifndef NEW_TOPO
void StartTopo(u8 func,u8 cnt,u8 interval,u8 *data,u8 datalen);
#else
void StartTopo(u32 sec1900,u16 pwm_l, u16 pwm_h, u16 ms, u8 *data, u8 datalen);
#endif
void StopTopo(void);
#endif

void PLC_StartCalibMeter(void);
void Open1Hz(void);
void Close1Hz(void);
void HRF_ReceivePhyCallback(uint8_t *data, uint16_t len, uint8_t crc_res, BPLC_recv_para *para);
void HRF_ReceiveCtrlCallback(uint8_t *data, BPLC_recv_para *para, bool_t status);
extern u32  Meter_SupportRecord;
#endif
