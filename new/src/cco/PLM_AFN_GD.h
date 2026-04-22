#ifndef PLM_AFN_GD_H
#define PLM_AFN_GD_H

#ifdef PROTOCOL_GD_2016

extern BYTE afn_func_01_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_02_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_03_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_04_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_06_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_07_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_08_gd(P_LMP_APP_INFO pLmpAppInfo);
extern BYTE afn_func_F0_gd(P_LMP_APP_INFO pLmpAppInfo);
unsigned short get_crc16(unsigned char *pbBuff, unsigned long dwLen);

#define DL645_SET_RECORD_INTERVAL(val) (0x40000A##val)
#define DL645_SET_RECORD_INTERVAL_33 (0x73333D00)
#define DL645_SET_RECORD_READ_33 (0x35003233)
#endif

#endif
