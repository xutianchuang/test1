#ifndef _HPLC_TEST_MODEL_H_
#define _HPLC_TEST_MODEL_H_


//测试模式
void PLC_TestModel(PDU_BLOCK* pdu);

//PLC透传模式
void PLC_ThroughModel(PDU_BLOCK* pdu);

//PLC回传模式
void PLC_BackModel(PDU_BLOCK* pdu);
//HRF透传模式
void HRF_BackModel(PDU_BLOCK* pdu);

//MSDU透传模式
void MSDU_ThroughModel(PDU_BLOCK* pdu);

//PLC到HRF回传模式
void Hplc_to_Hrf(PDU_BLOCK* pdu);
//测试模式校准
void PLC_TestModelMacCalibrateNTB(PDU_BLOCK* pdu);
int  PLC_SyncModelMacCalibrateNTB(PDU_BLOCK* pdu);
void HRF_TestModelMacCalibrateNTB(PDU_BLOCK* pdu);
int  HRF_SyncModelMacCalibrateNTB(PDU_BLOCK* pdu);
void Secure_TestMode(u8 mode ,u8 *data);
#endif