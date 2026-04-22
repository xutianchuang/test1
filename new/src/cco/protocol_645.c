
#include "system_inc.h"

void PLC_make_frame_645(P_GD_DL645_BODY pFrame, PBYTE pMeterNo, BYTE ctrl, PVOID pDataUnit, BYTE nDataLen)
{
    pFrame->protocolHead = pFrame->protocolHead2 = FRAME_DL645_START_FLAG;
    memcpy_special(pFrame->meter_number, pMeterNo, FRM_DL645_ADDRESS_LEN);
    pFrame->controlCode = ctrl;
    pFrame->dataLen = nDataLen;
    if(pDataUnit != NULL && nDataLen > 0)
    {
        memcpy_special(pFrame->dataBuf, pDataUnit, nDataLen);
    }
    AddValue(pFrame->dataBuf, nDataLen, 0x33);
    pFrame->dataBuf[nDataLen] = Get_checksum(&pFrame->protocolHead, nDataLen+10);
    pFrame->dataBuf[nDataLen+1] = FRAME_DL645_END_FLAG;
}

void PLC_make_frame_645_exception(P_GD_DL645_BODY pFrame, const P_GD_DL645_BODY pCmdDL645)
{
    BYTE ctrl = 0xd1;
    BYTE errIfo = 0x02;

    if((0x01==pCmdDL645->controlCode) && (2==pCmdDL645->dataLen))
    {
        ctrl = 0xc1;
        errIfo = 0x02;
    }
    else if((0x11==pCmdDL645->controlCode) && (4==pCmdDL645->dataLen))
    {
    }
    else
    {
        //USHORT read_sn = PLC_get_sn_from_addr(pCmdDL645->meter_number);
        //if(read_sn < CCTT_METER_NUMBER)
        {
            //if(METER_TYPE_PROTOCOL_97 == g_MeterRelayInfo[read_sn].meter_type.protocol)
            {
                ctrl = 0xc1;
                errIfo = 0x02;
            }
        }
    }

    PLC_make_frame_645(pFrame, pCmdDL645->meter_number, ctrl, &errIfo, 1);
}

P_GD_DL645_BODY PLC_check_dl645_frame(UCHAR* buf, USHORT nLen) //检查645帧是否合法
{
    P_GD_DL645_BODY pFrame = NULL;
    USHORT i;

    if(nLen <= 11)
        return NULL;

    for(i=0; i<nLen-11; i++)
    {
        pFrame = (P_GD_DL645_BODY)buf++;
        if(pFrame->protocolHead == FRAME_DL645_START_FLAG
           && pFrame->protocolHead2 == FRAME_DL645_START_FLAG
           && pFrame->dataLen <= (nLen-i-12)
           && pFrame->dataBuf[pFrame->dataLen] == Get_checksum(&pFrame->protocolHead, pFrame->dataLen+10)
           && pFrame->dataBuf[pFrame->dataLen+1] == FRAME_DL645_END_FLAG)
        {
            return pFrame;
        }
    }
    return NULL;
}




