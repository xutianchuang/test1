#ifndef _HPLC_FRAME_H_
#define _HPLC_FRAME_H_
#include "protocol_includes.h"

//组帧

//生成SOF中的必要参数
#define CreatSofParam(sofP,srcTei,dstTei,lid,broadcast,Nid,resendTimes,detect,link) \
do \
{\
    sofP->SrcTei = (srcTei);  \
    sofP->DstTei = (dstTei);\
    sofP->Lid = (lid);\
    sofP->BroadcastFlag = (broadcast);\
    sofP->NID = (Nid);\
    sofP->ResendTimes = resendTimes;\
	sofP->needChangeTMI = detect; \
	sofP->Link=link;\
	if (dstTei!=READER_TEI)\
	{\
		sofP->doubleSend = 1;\
	}\
}while(0)

//生成BPLC中的必要参数
#define CreateBPLCParam(bplcP,rssi,snr,ntb) \
do \
{\
    bplcP->Rssi = rssi;\
    bplcP->Snr = snr;\
    bplcP->sync_ntb = ntb;\
}while(0)

//生成MPDU帧头
#define CreateMPDUCtl(ctl,type,Nid) \
do \
{ \
    memset((ctl),0,sizeof(MPDU_CTL));\
    (ctl)->Type = (type);\
    (ctl)->ConnectInd = 1;\
    (ctl)->NID = Nid;\
    (ctl)->Zone0[11] |= (1 << 4);\
}while(0)

//判断MPDU帧头是否合法
bool IsMpduCtlVaild(MPDU_CTL *ctl);

//生成信标帧可变区域
#define CreateBeaconZone(beaconZone,Ntb,cnt,tei,copymodel,symbol,phase) \
do \
{\
    memset((beaconZone),0,sizeof(BEACON_ZONE));\
    beaconZone->NTB = (Ntb);\
    beaconZone->BeaconCnt = (cnt);\
    beaconZone->SrcTei = (tei);\
    beaconZone->CopyModel = (copymodel);\
    beaconZone->SymbolNum = (symbol);\
    beaconZone->Phase = (phase);\
    beaconZone->Version = 1;\
}while(0)


//生成SOF帧可变区域
#define CreateSOFZone(sofZone,srcTei,dstTei,lid,frameLen,block,symbol,boardcaseFlag,resendFlag,copy,copyex)\
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
    sofZone->CopyType = (copy); \
    sofZone->CopyExType = (copyex); \
    sofZone->Version = 1;\
}while(0)

//生成选择确认可变区域
#define CreateConfirmZone(confirmZone,result,state,dstTei,block) \
do \
{\
    memset((confirmZone),0,sizeof(CONFIRM_ZONE));\
    confirmZone->ReceiveResult = (result);   \
    confirmZone->ReceiveState = (state);  \
    confirmZone->DstTei = (dstTei);       \
    confirmZone->BlockNum = (block);      \
    confirmZone->Version = 1;\
	confirmZone->ExtType = 0;\
}while(0);

//生成网间协调帧的可变区域
#define CreateConcertZone(concertZone,continueTime,offset,Nid) \
do \
{\
    memset(concertZone,0,sizeof(CONCERT_ZONE));\
    concertZone->ContinueTime = (continueTime);   \
    concertZone->Tapeoffset = (offset);  \
    concertZone->NID = (Nid);\
}while(0)

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

//生成长MAC帧头
#define CreateLongMACHead(macHead,msduLen,dstTei,srcTei,netid,rebootNum,routeLeft,broadcast,sendType,sendLimit,msduSeq,dstMac)\
do \
{\
    memset((macHead),0,sizeof(MAC_HEAD_LONG));\
    macHead->MacHead.HeadType = 0;\
    macHead->MacHead.Version = 1;\
    macHead->MacHead.MsduLen = (msduLen);\
    macHead->MacHead.DstTei = (dstTei);\
    macHead->MacHead.SrcTei = (srcTei);\
    macHead->MacHead.NID = (netid);\
    macHead->MacHead.RebootNum = (rebootNum);\
    macHead->MacHead.RouteCnt = (routeLeft);\
    macHead->MacHead.Broadcast = (broadcast);\
    macHead->MacHead.SendType = (sendType);\
    macHead->MacHead.SendLimit = (sendLimit);\
    macHead->MacHead.MsduSeq = (msduSeq);\
    memcpy(macHead->DstMac,dstMac,6);\
}while(0)

//生成长MSDU帧头
#define CreateLongMsduHead(msduHead,dstMac,srcMac,vlan,msduType)\
do \
{\
    memcpy(msduHead->DstMac,dstMac,6);\
    memcpy(msduHead->SrcMac,srcMac,6);\
    msduHead->Vlan = vlan;\
    msduHead->MsduType = msduType;\
}while(0)

//生成短MAC帧头
#define CreateShortMACHead(macHead,msduLen,dstTei,srcTei,netid,rebootNum,routeLeft,broadcast,sendType,sendLimit,msduSeq)\
do \
{\
    memset((macHead),0,sizeof(MAC_HEAD_SHORT));\
    macHead->MacHead.HeadType = 1;\
    macHead->MacHead.Version = 1;\
    macHead->MacHead.MsduLen = (msduLen);\
    macHead->MacHead.DstTei = (dstTei);\
    macHead->MacHead.SrcTei = (srcTei);\
    macHead->MacHead.NID = (netid);\
    macHead->MacHead.RebootNum = (rebootNum);\
    macHead->MacHead.RouteCnt = (routeLeft);\
    macHead->MacHead.Broadcast = (broadcast);\
    macHead->MacHead.SendType = (sendType);\
    macHead->MacHead.SendLimit = (sendLimit);\
    macHead->MacHead.MsduSeq = (msduSeq);\
}while(0)

//生成短MSDU帧头
#define CreateShortMsduHead(msduHead,vlan,msduType)\
do \
{\
    msduHead->Vlan = vlan;\
    msduHead->MsduType = msduType;\
}while(0)

//生成管理帧头
#define CreateMMFrame(mmFrame,type)\
do \
{ \
    memset(mmFrame,0,sizeof(MME_FRAME));\
    mmFrame->Version = 1;\
    mmFrame->MMType = type;\
}while(0)


//生成关联请求
#define CreateAssocReq(assocPacket,mac,pco,pcoNum,phase1,phase2,phase3,devType,macType,staRand,selfDef,version,hardReboot,softReboot,netSeq,freType,p2p,hrf_hplc)\
do \
{\
    u8 i = 0;\
    memset((assocPacket),0,sizeof(ASSOC_REQ_PACKET));\
    memcpy(assocPacket->MacAddr,(mac),6);\
    for(i = 0;i<pcoNum;i++)\
    {\
        assocPacket->CandidataPCO[i].TEI = pco[i].TEI;\
		assocPacket->Link|=pco[i].Link<<i;\
    }\
    assocPacket->Phase1 = (phase1); \
	assocPacket->Phase2 = (phase2); \
	assocPacket->Phase3 = (phase3); \
    assocPacket->DeviceType = (devType);     \
    assocPacket->MacAddrType = (macType);    \
	assocPacket->IsHrfOrHplc = hrf_hplc;    \
    assocPacket->STARandom = (staRand);      \
    memcpy(assocPacket->FactoryMSG,(selfDef),18); \
    memcpy(&assocPacket->STAVersion,(version),sizeof(STATION_VERSION)); \
    assocPacket->STAVersion.SoftVersion = htons(assocPacket->STAVersion.SoftVersion); \
    assocPacket->STAVersion.FactoryCode = htons(assocPacket->STAVersion.FactoryCode); \
    assocPacket->STAVersion.ChipCode = htons(assocPacket->STAVersion.ChipCode); \
    assocPacket->HardReboot = (hardReboot);     \
    assocPacket->SoftReboot = (softReboot);    \
    assocPacket->PCOType = 0x02;   \
    assocPacket->NetSeq =  netSeq;\
    assocPacket->FreType = freType;\
    assocPacket->P2PSeq = (p2p);  \
    assocPacket->Version = 1; \
}while(0)

//生成关联指示
#define CreateAssocInd(assocIndPacket,result,layer,staMac,ccoMac,staTei,pcoTei,packetSeq,totalPacket,\
    endPacketFlg,random,netSeq,reAssocTime,p2p,routeData,routeDataSize)\
do \
{ \
    memset(assocIndPacket,0,sizeof(ASSOC_IND_PACKET));\
    assocIndPacket->Result = result;\
    assocIndPacket->Level = layer;\
    memcpy(assocIndPacket->STAMacAddr,staMac,6);\
    memcpy(assocIndPacket->CCOMacAddr,ccoMac,6);\
    assocIndPacket->STATei = staTei;\
    assocIndPacket->PCOTei = pcoTei;\
    assocIndPacket->PacketSeq = packetSeq;\
    assocIndPacket->TotalPacket = totalPacket;\
    assocIndPacket->EndPacketFlg = endPacketFlg;\
    assocIndPacket->StationRandom = random;\
    assocIndPacket->NetSeq = netSeq;\
    assocIndPacket->ReAssocTime = reAssocTime;\
    assocIndPacket->P2PSeq = p2p;\
    if(routeData)\
        memcpy(assocIndPacket->RouteData,routeData,routeDataSize);\
}while(0);
//关联请求报文是否合法
bool IsAssocReqVaild(ASSOC_REQ_PACKET *assocReq);

//生成心跳帧
#define CreateHeartBeatCheck(heartBeat,Tei,maxTei,maxNum,bitmap)\
do \
{\
    memset(heartBeat,0,sizeof(HEARTBEAT_CHECK_PACKET)-HEARTBEAT_BITMAP_SIZE-1);\
    heartBeat->SrcTei = (Tei);\
    heartBeat->MaxTei = (maxTei);\
    heartBeat->STANum = (maxNum);\
    if(bitmap)\
        memcpy(heartBeat->Bitmap,(bitmap),130);\
}while(0)

//心跳帧是否合法
bool IsHeartBeatVaild(HEARTBEAT_CHECK_PACKET *heartBeat);

//生成发现列表帧
#define CreateDiscoverNodeList(discoverPacket,Tei,PcoTEI,role,level,mac,finishFlg,pcoRate,pcoDownRate,\
                                num,sendNum,upNum,routeLeft,phase3,phase2,phase1,minRate,upData,bitmap,discoverMsg,discoverMsgNum)\
do \
{\
    memset(discoverPacket,0,(sizeof(DISCOVER_NODE_LIST_PACKET)-1));\
    discoverPacket->TEI = (Tei);\
    discoverPacket->PCOTei = (PcoTEI);\
    discoverPacket->Role = (role);\
    discoverPacket->Level = (level);\
    memcpy(discoverPacket->MacAddr,(mac),6);\
    discoverPacket->FinishRate = finishFlg;\
    discoverPacket->PCOSuccess = (pcoRate);\
    discoverPacket->PCODownSuccess = (pcoDownRate);\
    discoverPacket->STANum = (num);\
    discoverPacket->SendPacketNum = (sendNum);\
    discoverPacket->UpRouteNum = (upNum);\
    discoverPacket->RouteItemLen = 8;\
    discoverPacket->RoutePeriodLeft = (routeLeft);\
    discoverPacket->Phase3 = (phase3);\
    discoverPacket->Phase2 = (phase2);\
    discoverPacket->Phase1 = (phase1);\
    discoverPacket->MinSuccess = (minRate);\
    UP_ROUTE_FILED *upRoute = (UP_ROUTE_FILED*)&discoverPacket->Data[0];\
    if(upData)\
        memcpy(upRoute,(upData),discoverPacket->UpRouteNum*sizeof(UP_ROUTE_FILED));\
    u8* bitmapZone = &discoverPacket->Data[discoverPacket->UpRouteNum*sizeof(UP_ROUTE_FILED)];\
    if(bitmap)\
        memcpy(bitmapZone,(bitmap),DISCOVER_BITMAP_SIZE);\
    u8* discoverZone = &bitmapZone[DISCOVER_BITMAP_SIZE];\
    if(discoverMsg)\
        memcpy(discoverZone,(discoverMsg),discoverMsgNum);\
}while(0)

//发现列表是否合法
bool IsDiscoverPacketVaild(DISCOVER_NODE_LIST_PACKET* discoverPacket);

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

//生成代理变更请求报文
#define CreateChangeProxyReq(cProxyReq,Tei,candicatePco,pcoNum,oldPco,reason,phase1,phase2,phase3,seq,netSeq)  \
do \
{\
    u8 i = 0;\
    memset(cProxyReq,0,sizeof(CHANGE_PROXY_REQ_PACKET));\
    cProxyReq->TEI = Tei;\
    for(;i<pcoNum;i++)\
	{\
        cProxyReq->CandidataPCO[i].TEI = candicatePco[i].TEI;\
		cProxyReq->LinkType |= candicatePco[i].Link<<i;\
	}\
    cProxyReq->OldPCOTei = oldPco;\
    cProxyReq->ProxyType = 0x02;\
    cProxyReq->Reason = reason;\
    cProxyReq->Phase1 = phase1;\
    cProxyReq->Phase2 = phase2;\
    cProxyReq->Phase3 = phase3;\
    cProxyReq->P2PSeq = seq;\
    cProxyReq->NetSeq = netSeq;\
}while(0)

//生成应用层通用报文格式帧
#define CreateAppPacket(appPacket)\
do \
{\
    appPacket->Port = 0x11;\
    appPacket->PacketID = 0x101;\
    appPacket->Reserve = 0;\
}while(0)

//生成应用业务报文
#define CreateAppBusinessPacket(appGeneralPacket,frameType,exZoneFlg,ackFlg,startFlg,dir,\
                                busFlg,appSeq,appFrameLen,data)\
do \
{\
    appGeneralPacket->Ctrl.FrameType = frameType;\
    appGeneralPacket->Ctrl.Reserve = 0;\
    appGeneralPacket->Ctrl.ExZone = exZoneFlg;\
    appGeneralPacket->Ctrl.AckFlg = ackFlg;\
    appGeneralPacket->Ctrl.StartFlg = startFlg;\
    appGeneralPacket->Ctrl.TranDir = dir;\
    appGeneralPacket->BusFlg = busFlg;\
    appGeneralPacket->Version = 0x01;\
    appGeneralPacket->AppSeq = appSeq;\
    appGeneralPacket->AppFrameLen = appFrameLen;\
    if(data)\
        memcpy(appGeneralPacket->Data,data,appFrameLen);    \
}while(0)                     

//生成数据透传上行帧
#define CreateReadMeterAck(readMeter,srcMac,dstMac,dataLen,data)\
do \
{\
    memcpy(readMeter->SrcMac,srcMac,6);\
    memcpy(readMeter->DstMac,dstMac,6);\
    readMeter->Reserve = 0;\
    readMeter->DataLen = dataLen;\
    if(data)\
        memcpy(readMeter->Data,data,dataLen);\
}while(0)

//生成搜表上行帧
#define CreateScanUpFrame(scanFrame,meterNum,data) \
do \
{\
    scanFrame->MeterNum = meterNum;\
    memset(scanFrame->Reserve,0,3);\
    if(data)\
        memcpy(scanFrame->Data,data,sizeof(APP_METER_INFO)*meterNum);\
}while(0)

//生成事件上报帧
#define CreateEventReportFrame(eventFrame,meterMac,data,len) \
do \
{\
    memcpy(eventFrame->MeterMac,meterMac,6);\
    if(data)\
        memcpy(eventFrame->Data,data,len);\
}while(0)

//生成停电事件上报帧
#define CreatePowerEventReportFrame(powerEventFrame,func,meterMac,data,len) \
do \
{\
	powerEventFrame->HeadLen = 12;		\
	powerEventFrame->Function = func;	   \
	powerEventFrame->DataLen = len;		\
	memcpy(&powerEventFrame->MeterAddr[0],meterMac,6);\
	if(data)\
		memcpy(&powerEventFrame->Data[0],data,len);\
}while(0);

//生成升级包头
#define CreateUpdateHeadFrame(sendUpdatePacket,updateInfoID)\
do \
{\
    sendUpdatePacket->UpdateInfoID = updateInfoID;\
    memset(sendUpdatePacket->Reserve,0,3);\
}while(0)

//生成升级信息上行帧
#define CreateUpdateInfoFrame(updateFrame,updateID,result,error) \
do \
{ \
    updateFrame->UpdateID = updateID;\
    updateFrame->ResultCode = result;\
    updateFrame->ErrorCode = error;\
}while(0)

//生成台区户变关系头
#define CreateZonePacketHeadFrame(zoneFrame,phase,mac,feature,collect) \
do \
{\
    zoneFrame->HeadLen = 12;\
	zoneFrame->CollectPhase = phase;\
	memcpy(zoneFrame->MacAddr,mac,6);\
	zoneFrame->FeatureType = feature;\
	zoneFrame->CollectType = collect;\
	memset(zoneFrame->Reserve2,0,sizeof(zoneFrame->Reserve2));\
}while(0)

//生成台区告知报文
#define CreateZoneInformFrame(zoneFrame,tei,type,seq,num,ntb) \
do \
{\
	zoneFrame->TEI = tei;\
	zoneFrame->CollectType = type;\
	zoneFrame->Reserve1 = 0;\
	zoneFrame->CollectSeq = seq;\
	zoneFrame->InformNum = num;\
	zoneFrame->BeginNTB = ntb;\
}while(0)

//生成台区告知内容报文头
#define CreateZoneInformDataFrame(zoneFrame,num1,num2,num3) \
do \
{\
	zoneFrame->Reserve = 0;\
	zoneFrame->PhasexNum[0] = num1;\
	zoneFrame->PhasexNum[1] = num2;\
	zoneFrame->PhasexNum[2] = num3;\
}while(0)

//生成台区告知内容报文头
#define CreateZoneResultFrame(zoneFrame,tei,flag,result,ccoMac) \
do \
{ \
	zoneFrame->TEI = tei;\
	zoneFrame->Flag = flag;\
	zoneFrame->Result = result;\
	if(ccoMac)\
		memcpy(zoneFrame->CCOAddr,ccoMac,6);\
}while(0)


#endif
