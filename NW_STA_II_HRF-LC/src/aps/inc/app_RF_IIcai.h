#ifndef _APP_RF_IICAI_H
#define _APP_RF_IICAI_H
#if 0
//add by LiHuaQiang 2020.10.10 -START-
#define SELF_DEFINE                   0x00
#define READ_DATA_97                  0x01      
#define READ_DATA_07                  0x11  
#define WRITE_DATA_97                 0x04       
#define WRITE_DATA_07                 0x14    
#define REQUEST_DEVICE_ADDR_07        0x13    
#define WRITE_DEVICE_ADDR_07          0x15   
#define PROTOCOL_COAT_SIZE            12      //645/1997??????????(0x68+address+0x68+controlbyte+len......cs+0x16)
#define CS_POSITION                   10      //645/1997?????????cs??????
#define END_FLAG_POSITION             11      //645/1997???????????????
#define END_FLAG                      0x16    //645/1997????????

#define LOCAL_IFR_BUFF_SIZE           500

#define BAUD_IFR                      1200    //?????

#define offset_of(obj_type,mb)  ((UINT16)&(((obj_type*)0)->mb))

void AppLocalHandleIFR(void);
void AnswerReadMeter2IFR(u8 *data,u16 len);
bool GetProduceModeFlag(void);
void memadd(UINT8 buf[], UINT8 addend, UINT8 cnt);
bool GetProduceModeFlag(void);
#endif
//add by LiHuaQiang 2020.10.10 -END-
#endif
