#include "debug_cmd.h"
#include "protocol_includes.h"
#include "common_includes.h"
#include "finsh.h"

//所有命令在此添加

static bool DebugSwitch = false;

//打印版本
long version(void)
{
	
    return 0;
}
MSH_CMD_EXPORT(version, show STA version information);



//long nr(int argc ,char** argv)
//{
//	const ST_NEIGHBOR_TAB_TYPE* neighborPt = GetNeighborTable();
//	static const char *sTEI = "TEI";
//	static const char *sPcoTEI = "PCO";
////	static const char *sRole = "Role";
////	static const char *sPhase = "Phase";
//	static const char *sUpRate = "UpRate";
//	static const char *sDownRate = "DownRate";
//	static const char *sLayer = "Layer";
//	static const char *sSNR = "SNR";
//	static const char *sRssi = "RSSI";
//	static const char *sMac = "Mac";
//
//	static const char *slastSend = "LastSen";
//	static const char *slastRec = "LastRec";
//	static const char *slastTRec = "lastTRec";
//
//
//	if(strcmp(argv[1],"-t") == 0)
//	{
//		u8 upRate = 0;
//        u8 downRate = 0;
//		u8 rate = 0;
//		u16 i = 0;
//		rt_kprintf("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\r\n",
//					   sTEI,sPcoTEI,sUpRate,sDownRate,sLayer,sSNR,sRssi,slastSend,slastRec,slastTRec,sMac);
//		for(;i<neighborPt->NeighborNum;i++)
//		{
//			u16 TEI = neighborPt->TEI[i];
//			char StringAddr[13];
//			GetAddrString(neighborPt->Neighbor[TEI].Mac,StringAddr);
//			GetNeighborSucRate(TEI,&upRate,&downRate,&rate,true,2);
//			rt_kprintf("%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%8d\t%s\r\n",
//					   TEI,
//					   neighborPt->Neighbor[TEI].PcoTEI,
//					   upRate,
//					   downRate,
//					   neighborPt->Neighbor[TEI].Layer,
//					   neighborPt->Neighbor[TEI].SNR,
//					   neighborPt->Neighbor[TEI].RSSI,
//					   neighborPt->Neighbor[TEI].LastNghTxCnt[0],
//					   neighborPt->Neighbor[TEI].LastNghRxCnt[0],
//					   neighborPt->Neighbor[TEI].LastTheStaRxCnt[0],
//					   StringAddr);
//		}
//	}
//	else
//	{
//		u16 i = 0;
//		for(;i<neighborPt->NeighborNum;i++)
//		{
//			rt_kprintf("TEI:%d\r\n",neighborPt->TEI[i]);
//		}
//	}
//	return 0;
//}
//MSH_CMD_EXPORT(nr, show neighbor information <-t>);


long np(void)
{
	const ST_STA_PERIOD_TYPE * pPeriodPara = GetStaPeriodPara();
	static const char* sRoutePeriod = "RoutePeriod";
	static const char* sLeftRoute = "LeftRoute";
	static const char* sDisPeriod = "DisPeriod";
	static const char* sSendDis = "SendDis";
	static const char* sLastSendDis = "LastSendDis";
	
	rt_kprintf("%12s\t%12s\t%12s\t%12s\t%12s\r\n",
				sRoutePeriod,sLeftRoute,sDisPeriod,sSendDis,sLastSendDis);
	rt_kprintf("%12d\t%12d\t%12d\t%12d\t%12d\r\n",
			   pPeriodPara->RoutingPeriod,RouteTimeLeft(),pPeriodPara->StaDiscListPeriod,ThisSendDiscoverNum(),pPeriodPara->LastPeriodDiscListTxCnt[0]);
	return 0;
}
MSH_CMD_EXPORT(np, show route information);


//long nid(void)
//{
//	rt_kprintf("Warning:Only use in offline\r\n");
//	PrintAllNID();
//	return 0;
//}
//MSH_CMD_EXPORT(nid, show NID information);

long ol(void)
{
	const char* reason = GetLastOfflineReason();
	rt_kprintf("%s\r\n",reason);
	return 0;
}
MSH_CMD_EXPORT(ol, show last offline reason);

long sf(int argc ,char** argv)
{
	if(argv[1][0] < '0' || argv[1][0] > '3')
	{
		rt_kprintf("Invaild Frequence:%s\r\n",argv[1]);
		return 0;
	}
	u8 fre = HPLC_PHY_GetFrequence();
	if(argv[1][0] - '0' != fre)
	{
		HPLC_PHY_SetFrequence(argv[1][0] - '0');
		ResetCalibrateNTB();
	}
	rt_kprintf("Change Frequence to :%s\r\n",argv[1]);
	return 0;
}
MSH_CMD_EXPORT(sf, Set Frequence);

long debug(int argc ,char** argv)
{
	if(argv[1][0] == '1')
	{
		rt_kprintf("Debug Open!\r\n");
		DebugSwitch = true;
	}
	else if(argv[1][0] == '0')
	{
		rt_kprintf("Debug Close!\r\n");
		DebugSwitch = false;
	}
	return 0;
}
MSH_CMD_EXPORT(debug, Debug Switch);

long reboot(void)
{
	if(DebugSwitch)
	{
		rt_kprintf("System Reboot rightnow!\r\n");
		RebootSystem();
	}
	else
	{
		rt_kprintf("Debug Switch is close now,please enter debug 1 first\r\n");
	}
	return 0;
}
MSH_CMD_EXPORT(reboot, Reboot system);

long offline(void)
{
	if(DebugSwitch)
	{
		rt_kprintf("offline rightnow!\r\n");
		DebugOffline();
	}
	else
	{
		rt_kprintf("Debug Switch is close now,please enter debug 1 first\r\n");
	}
	return 0;
}
MSH_CMD_EXPORT(offline, make the sta offline);

extern u32 HPLCCurrentFreq;

long smode(void)
{
	rt_kprintf("show mode ch=%d; mode=%d\r\n",HPLCCurrentFreq,GetAppCurrentModel());
	return 0;
}
MSH_CMD_EXPORT(smode, show mode);


