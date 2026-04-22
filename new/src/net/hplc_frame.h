#ifndef _HPLC_FRAME_H_
#define _HPLC_FRAME_H_
#include "System_inc.h"

//组帧

//生成SOF中的必要参数
#define CreatSofParam(sofP,srcTei,dstTei,lid,broadcast,Nid,resendTimes) \
do \
{\
    sofP->SrcTei = (srcTei);  \
    sofP->DstTei = (dstTei);\
    sofP->Lid = (lid);\
    sofP->BroadcastFlag = (broadcast);\
    sofP->NID = (Nid);\
    sofP->ResendTimes = resendTimes;\
}while(0)


//生成MPDU帧头
#ifdef HPLC_CSG

#define CreateMPDUCtl(ctl,type,Nid) \
do \
{ \
    memset((ctl),0,sizeof(MPDU_CTL));\
    (ctl)->Type = (type);\
    (ctl)->NetType = 1;\
    (ctl)->NID = (Nid);\
    (ctl)->Zone0[11]|=(0x1 << 4);\
}while(0)

#else

#define CreateMPDUCtl(ctl,type,Nid) \
do \
{ \
    memset((ctl),0,sizeof(MPDU_CTL));\
    (ctl)->Type = (type);\
    (ctl)->NetType = 0;\
    (ctl)->NID = (Nid);\
}while(0)

#endif

//生成信标帧可变区域
#ifdef HPLC_CSG

#define CreateBeaconZone(beaconZone,Ntb,tei,copymodel,symbol,phase) \
do \
{\
    memset((beaconZone),0,sizeof(BEACON_ZONE));\
    beaconZone->NTB = (Ntb);\
    beaconZone->SrcTei = (tei);\
    beaconZone->CopyModel = (copymodel);\
    beaconZone->SymbolNum = (symbol);\
    beaconZone->Phase = (phase);\
    beaconZone->Version = (1);\
}while(0)

#else

#define CreateBeaconZone(beaconZone,Ntb,tei,copymodel,symbol,phase) \
do \
{\
    memset((beaconZone),0,sizeof(BEACON_ZONE));\
    beaconZone->NTB = (Ntb);\
    beaconZone->SrcTei = (tei);\
    beaconZone->CopyModel = (copymodel);\
    beaconZone->SymbolNum = (symbol);\
    beaconZone->Phase = (phase);\
}while(0)

#endif

//生成SOF帧可变区域
#ifdef HPLC_CSG

#define CreateSOFZone(sofZone,srcTei,dstTei,lid,frameLen,block,symbol,boardcaseFlag,resendFlag,encrypt,copy,copyex)\
do \
{\
    memset((sofZone),0,sizeof(SOF_ZONE));\
    sofZone->SrcTei = (srcTei);\
    sofZone->DstTei = (dstTei);\
    sofZone->LinkFlag = (lid);\
    sofZone->FrameLen = (frameLen); \
    sofZone->PhyBlock = (block);  \
    sofZone->SymbolNum = (symbol);\
    sofZone->BroadcastFlag = boardcaseFlag; \
    sofZone->ReSendFlag = (resendFlag);\
    sofZone->CopyType = (copy); \
    sofZone->CopyExType = (copyex); \
    sofZone->Version = (1);\
}while(0)

#else

#define CreateSOFZone(sofZone,srcTei,dstTei,lid,frameLen,block,symbol,boardcaseFlag,resendFlag,encrypt,copy,copyex)\
do \
{\
    memset((sofZone),0,sizeof(SOF_ZONE));\
    sofZone->SrcTei = (srcTei);\
    sofZone->DstTei = (dstTei);\
    sofZone->LinkFlag = (lid);\
    sofZone->FrameLen = (frameLen); \
    sofZone->PhyBlock = (block);  \
    sofZone->SymbolNum = (symbol);\
    sofZone->BroadcastFlag = (boardcaseFlag); \
    sofZone->ReSendFlag = (resendFlag);\
    sofZone->EncryptFlag = (encrypt);\
    sofZone->CopyType = (copy); \
    sofZone->CopyExType = (copyex); \
}while(0)

#endif

//生成选择确认可变区域
#ifdef HPLC_CSG

#define CreateConfirmZone(confirmZone,result,state,srcTei,dstTei,block,quality,stationLoad) \
do \
{\
    memset((confirmZone),0,sizeof(CONFIRM_ZONE));\
    confirmZone->ReceiveResult = (result);   \
    confirmZone->ReceiveState = (state);  \
    confirmZone->DstTei = (dstTei);       \
    confirmZone->BlockNum = (block);      \
    confirmZone->Version = (1);\
	confirmZone->ExtType=0;\
}while(0);

#else

#define CreateConfirmZone(confirmZone,result,state,srcTei,dstTei,block,quality,stationLoad) \
do \
{\
    memset((confirmZone),0,sizeof(CONFIRM_ZONE));\
    confirmZone->ReceiveResult = (result);   \
    confirmZone->ReceiveState = (state);  \
    confirmZone->SrcTei = (srcTei);       \
    confirmZone->DstTei = (dstTei);       \
    confirmZone->BlockNum = (block);      \
    confirmZone->ChannelQuality = (quality); \
    confirmZone->StationLoad = (stationLoad); \
    confirmZone->ExtType = 0;   \
}while(0);

#endif

//生成网间协调帧的可变区域
#ifdef HPLC_CSG

#define CreateConcertZone(concertZone,continueTime,offset,Nid) \
do \
{\
    memset(concertZone,0,sizeof(CONCERT_ZONE));\
    concertZone->ContinueTime = (continueTime);   \
    concertZone->Tapeoffset = (offset);  \
    concertZone->NID = (Nid);\
    concertZone->Version = (1);\
}while(0)

#else

#define CreateConcertZone(concertZone,continueTime,offset,Nid) \
do \
{\
    memset(concertZone,0,sizeof(CONCERT_ZONE));\
    concertZone->ContinueTime = (continueTime);   \
    concertZone->Tapeoffset = (offset);  \
    concertZone->NID = (Nid);\
}while(0)

#endif

//生成CRC32校验
#define CreateMACCrc32(macFrame,len) \
do \
{\
    _CRC_32_ crc32 = GetCrc32((u8*)macFrame,(len));\
    memcpy((u8*)macFrame + (len),&crc32,4);\
}while(0)

//生成CRC24校验
#define CreatePhyCrc24(data,len) \
do \
{\
    _CRC_24_ crc24 = GetCrc24((u8*)data,(len));\
    memcpy((u8*)data + (len),&crc24,3);\
}while(0)

//生成MAC帧头
#define CreateMACHead(macHead,srcTei,dstTei,sendType,sendLimit,msduSeq,msduType,msduLen,rebootNum,pcoFlag,\
                        routeTotal,routeLeft,broadcast,repair,macFlag,netSeq,srcMac,dstMac) \
do \
{\
    memset((macHead),0,sizeof(MAC_HEAD));\
    macHead->SrcTei = (srcTei);\
    macHead->DstTei = (dstTei);\
    macHead->SendType = (sendType);\
    macHead->SendLimit = (sendLimit);\
    macHead->MsduSeq = (msduSeq);\
    macHead->MsduType = (msduType);\
    macHead->MsduLen = (msduLen);\
    macHead->RebootNum = (rebootNum);\
    macHead->PCORouteFlag = (pcoFlag);\
    macHead->RouteSum = (routeTotal);\
    macHead->RouteLeft = (routeLeft);\
    macHead->Broadcast = (broadcast);\
    macHead->RouteRepair = (repair);\
    macHead->MACFlag = (macFlag);\
    macHead->NetSeq = (netSeq);\
    if(macHead->MACFlag)\
    {\
        memcpy(macHead->SrcMac,(srcMac),6);\
        memcpy(macHead->DstMac,(dstMac),6);\
    }\
}while(0)

//生成关联请求
#define CreateAssocReq(assocPacket,mac,pco,pcoNum,phase,devType,macType,staRand,selfDef,version,hardReboot,softReboot,p2p,chipID)\
do \
{\
    u8 i = 0;\
    memset((assocPacket),0,sizeof(ASSOC_REQ_PACKET));\
    memcpy(assocPacket->MacAddr,(mac),6);\
    for(i = 0;i<pcoNum;i++)\
    {\
        assocPacket->CandidataPCO[i].TEI = pco[i];\
    }\
    assocPacket->PhaseLine = (phase); \
    assocPacket->DeviceType = (devType);     \
    assocPacket->MacAddrType = (macType);    \
    assocPacket->STARandom = (staRand);      \
    memcpy(assocPacket->FactoryMSG,(selfDef),18); \
    memcpy(&assocPacket->STAVersion,(&version),sizeof(STATION_VERSION)); \
    assocPacket->HardReboot = (hardReboot);     \
    assocPacket->SoftReboot = (softReboot);    \
    assocPacket->PCOType = 0;   \
    assocPacket->P2PSeq = (p2p);  \
    memcpy(&assocPacket->ChipID,chipID,24);\
}while(0)

//生成心跳帧
#define CreateHeartBeatCheck(heartBeat,Tei,maxTei,maxNum,bitmapSize,bitmap)\
do \
{\
    heartBeat->SrcTei = (Tei);\
    heartBeat->Reserve1 = 0;\
    heartBeat->MaxTei = (maxTei);\
    heartBeat->Reserve2 = 0;\
    heartBeat->STANum = (maxNum);\
    heartBeat->BitmapSize = (bitmapSize);\
    if(bitmap)\
        memcpy(heartBeat->Bitmap,(bitmap),heartBeat->BitmapSize);\
}while(0)

//生成发现列表帧
#define CreateDiscoverNodeList(discoverPacket,Tei,PcoTEI,role,level,mac,ccoMac,phase,pcoSNR,pcoRate,pcoDownRate,\
                                num,sendNum,upNum,routeLeft,bitmapSize,minRate,upData,bitmap,discoverMsg,discoverMsgNum)\
do \
{\
    discoverPacket->TEI = (Tei);\
    discoverPacket->PCOTei = (PcoTEI);\
    discoverPacket->Role = (role);\
    discoverPacket->Level = (level);\
    memcpy(discoverPacket->MacAddr,(mac),6);\
    memcpy(discoverPacket->CCOMacAddr,(ccoMac),6);\
    discoverPacket->Phase = (phase);\
    discoverPacket->Reserve1 = 0;\
    discoverPacket->PCO_SNR = (pcoSNR);\
    discoverPacket->PCOSuccess = (pcoRate);\
    discoverPacket->PCODownSuccess = (pcoDownRate);\
    discoverPacket->STANum = (num);\
    discoverPacket->SendPacketNum = (sendNum);\
    discoverPacket->UpRouteNum = (upNum);\
    discoverPacket->RoutePeriodLeft = (routeLeft);\
    discoverPacket->BitmapSize = (bitmapSize);\
    discoverPacket->MinSuccess = (minRate);\
    memset(discoverPacket->Reserve2,0,3);\
    UP_ROUTE_FILED *upRoute = (UP_ROUTE_FILED*)&discoverPacket->Data[0];\
    if(upData)\
        memcpy(upRoute,(upData),discoverPacket->UpRouteNum*sizeof(UP_ROUTE_FILED));\
    u8* bitmapZone = &discoverPacket->Data[discoverPacket->UpRouteNum*sizeof(UP_ROUTE_FILED)];\
    if(bitmap)\
        memcpy(bitmapZone,(bitmap),discoverPacket->BitmapSize);\
    u8* discoverZone = &bitmapZone[discoverPacket->BitmapSize];\
    if(discoverMsg)\
        memcpy(discoverZone,(discoverMsg),discoverMsgNum);\
}while(0)

//生成成功率上报帧
#define CreateSuccessRateReport(successRate,Tei,staNum,msg) \
do \
{\
    successRate->TEI = Tei;\
    successRate->Reserve1 = 0;\
    successRate->STANum = staNum;\
    if(msg)\
        memcpy(successRate->SuccessRate,msg,sizeof(SUCCESS_RATE_FILED)*staNum);\
}while(0)


//生成应用层通用报文格式帧
#define CreateAppGeneralPacket(generalPacket,port,id)\
do \
{\
    generalPacket->Port = port;\
    generalPacket->PacketID = id;\
    generalPacket->Ctl = 0;\
}while(0)

//生成抄表上行帧
#define CreateReadMeterAck(readMeter,headLen,protocol,dataLen,packetSeq,item,data)\
do \
{\
    readMeter->AppProtocol = 1;\
    readMeter->HeadLen = headLen;\
    readMeter->ReplyState = 0;\
    readMeter->Protocol = protocol;\
    readMeter->DataLen = dataLen;\
    readMeter->PacketSeq = packetSeq;\
    readMeter->Item = item;\
    memcpy(readMeter->Data,data,dataLen);\
}while(0)

#endif
