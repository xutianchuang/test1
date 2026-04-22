#include "protocol_includes.h"
#include "common_includes.h"
extern int lcdiff_update(uint32_t download_addr, uint32_t download_area_size);

#define DEFAULT_FILE_CRC        0xF91A3B41       //默认CRC
#define DEFAULT_FILE_SIZE       (312*1024UL)      //默认文件大小
#define DEFAULT_SOFT_VERSION    0x01             //默认软件版本
#define DEFAULT_HARD_VERSION    0x01             //默认硬件版本

#define MAX_UPDATE_FILE_SIZE    (FLASH_CODE1_SIZE)
#define MAX_UPDATEMAPBITSIZE             (MAX_UPDATE_FILE_SIZE/100 + 1)

u8 last_up_status=0;
//#define GW_TEST_VERSION

#define APP_CREATE_UPDATE_DEF() \
MAC_HEAD *macHead = (MAC_HEAD*)arg;\
APP_PACKET *appPacket = 0;\
APP_GENERAL_PACKET *appGeneral = 0;\
if(macHead->HeadType)\
    appPacket = (APP_PACKET*)((u8*)macHead + sizeof(MAC_HEAD_SHORT) + sizeof(MSDU_HEAD_SHORT));\
else \
    appPacket = (APP_PACKET*)((u8*)macHead + sizeof(MAC_HEAD_LONG) + sizeof(MSDU_HEAD_LONG));\
appGeneral = (APP_GENERAL_PACKET*)&appPacket->Data[0];\
APP_UPDATE_PACKET *updatePacket = (APP_UPDATE_PACKET*)&appGeneral->Data[0];

//升级状态
typedef enum
{
    UPDATE_STATE_INVAILD,     //无效
    UPDATE_STATE_IDLE,        //空闲
    UPDATE_STATE_RECEIVING,   //接收进行态
    UPDATE_STATE_FINISH,      //接收完成态
    UPDATE_STATE_EXECUTE,     //升级进行态
}UPDATE_STATE;

#ifdef BLE_NET_ENABLE
typedef enum
{
	LC_FILE_INIT = 0,
	LC_FILE_UNKNOW = 1,
	LC_FILE_WRITING = 2,
	LC_FILE_FINISH = 3,
}lc_file_state_type;
void(*lc_file_state_cbfun )(lc_file_state_type)  = NULL;
uint32_t (*lc_app_file_check_cbfun)() = NULL;
#endif


#pragma pack(4)
typedef struct
{
    u32 UpdateTimeout;      //升级时间窗
    u32 CurrentUpdateID;    //当前升级ID
    u32 CurrentTotalFileSize;   //当前升级文件总大小
    u32 CurrentFileSize;    //当前收到的文件大小
    u16 CurrentTotalSegNum; //文件总段数
	u8  IsOwnFile;          //是否是自家模块
	u32 CurrentFileCRC;     //当前文件CRC
    u32 CurrentFileSegSize; //文件段大小
    u8  FilePro;          //文件类型(0x02:从节点模块文件,0x03:采集器文件,0x04:电能表文件)
    
    u8 FileBitmap[MAX_UPDATEMAPBITSIZE];    //文件位图
    u16 FileBitmapSize;     //位图大小
    u32 MaxBlockSeq;        //最大数据块序号
	
	u32 UpdateUserAddr;		//当前需要改写的用户特征区地址
	u32 UpdateUserSize;		//用户特征区大小
	u32 UpdateCodeAddr;		//当前擦除和编程的FLASH区地址
	u32 UpdataCodeSize;		//代码区大小
	
	u8  EraseBlockBitmap[128];		//标记哪些块在本次升级中已经执行擦除
}UpdateParam;

#pragma pack(4)
typedef struct
{
    u32 DebugCRC;           
    u32 DebugFileSize;
    u8  DebugSoftVersion;
    u8  DebugHardVersion;
}DEBUG_UPDATE_INFO;

//升级状态机
static FSMSystem UpdateFSM;

//升级参数
static UpdateParam CurrentUpdateParam;

//升级超时定时器
static TimeValue UpdateTimer;

//试运行时间
//static u32 TryRunTime = 0;

void setIsOwnFileFlag()
{
	CurrentUpdateParam.IsOwnFile = 1;
}

//调试数据
DEBUG_UPDATE_INFO DebugUpdateInfo;

#define InitDebugUpdateInfo()\
do \
{\
    DebugUpdateInfo.DebugCRC = DEFAULT_FILE_CRC;\
    DebugUpdateInfo.DebugFileSize = DEFAULT_FILE_SIZE;\
    DebugUpdateInfo.DebugHardVersion = DEFAULT_HARD_VERSION;\
    DebugUpdateInfo.DebugSoftVersion = DEFAULT_SOFT_VERSION;\
}while(0);

//空闲运行
void UpdateIdleRun(void *fsm,void* arg)
{
    
}

//接收进行态
void UpdateReceivingRun(void *fsm,void* arg)
{
    if(TimeOut(&UpdateTimer))   //超时，返回空闲
    {
        FSMSystem *this = (FSMSystem *)fsm;
        memset(&CurrentUpdateParam,0,sizeof(CurrentUpdateParam));
        debug_str(DEBUG_LOG_UPDATA, "UpdateReceivingRun, timeout\r\n");
        FSMChangeState(this,UPDATE_STATE_IDLE);
    }
}


void DoUpdate(u32 tryTime)
{
#ifndef GW_TEST_VERSION
	UserSection_Type userSection;
	if (!CurrentUpdateParam.IsOwnFile)
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff IsOwnFile err\r\n");
		last_up_status=0x1;
		return;
	}
	userSection.Version = CurrentCodeSection->Version;
	//升级
	
	AplCommonParam_type paramType;
	FlashTryRunParam tryRunParam;
	//读取用户特征字
//	ReadFlash(CurrentUpdateParam.UpdateUserAddr,(u8*)&userSection,sizeof(UserSection_Type));
	//读取代码CRC校验、版本等信息
	ReadFlash((CurrentUpdateParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE),(u8*)&paramType,sizeof(AplCommonParam_type));
	//读取试运行参数
	ReadFlash(TRY_RUN_TIME_ADDRESS,(u8*)&tryRunParam,sizeof(FlashTryRunParam));
	
	//把代码版本中的信息写到用户特征字中
	#if defined(ZB204_CHIP)
	userSection.CopyToRamFlg = 1;
	#endif
	userSection.CodeSize = paramType.CodeLength;
	userSection.Version++;//paramType.AplVersion;
	userSection.CodeSpiFlashStartAddr=CurrentUpdateParam.UpdateCodeAddr-FLASH_START_ADDR;
	userSection.FeatureCode = USER_FEATURE_CODE;
	userSection.CodeCheckSum = CheckU32Sum((u32*)CurrentUpdateParam.UpdateCodeAddr,paramType.CodeLength/4);
	//userSection.CodeCheckSum = ~userSection.CodeCheckSum;
	userSection.UserSectionCheckSum = CheckU32Sum((u32*)&userSection,(sizeof(UserSection_Type) - 4)/4);
	
	DataFlashInit();
	Unprotection1MFlash();
	EraseFlash(CurrentUpdateParam.UpdateUserAddr,sizeof(UserSection_Type));		
	WriteFlash(CurrentUpdateParam.UpdateUserAddr, (u8*)&userSection, sizeof(UserSection_Type));
	Protection1MFlash();
	//把试运行时间写入到参数中

	tryRunParam.Param1.TryRunTime = tryTime;
	tryRunParam.Param1.TryRunVersion = paramType.AplVersion;//userSection.Version;
	tryRunParam.Param1.FileSize = CurrentUpdateParam.CurrentTotalFileSize;
	tryRunParam.Param1.FileCRC = CurrentUpdateParam.CurrentFileCRC;

	tryRunParam.CRC32 = CommonGetCrc32((u8*)&tryRunParam,sizeof(FlashTryRunParam) - 4);
	EraseFlash(TRY_RUN_TIME_ADDRESS,sizeof(FlashTryRunParam));
	WriteFlash(TRY_RUN_TIME_ADDRESS, (u8*)&tryRunParam, sizeof(FlashTryRunParam));
	DataFlashClose();
	
	CurrentUpdateParam.CurrentTotalFileSize = paramType.CodeLength;
	SYS_UP_ReWrite((UPDATA_PARA *)(CurrentUpdateParam.UpdateCodeAddr + CurrentUpdateParam.CurrentTotalFileSize - sizeof(UPDATA_PARA)));
#endif
}

//按地址擦除
//0x12003000-7FFFF
//0x12008000-FCFFF

static void EraseUpdateBlock(u32 beginAddr,u32 size)
{
	//先计算当前块剩余空间数
	if(size < 1)
		return;
	u32 offset = beginAddr - CurrentUpdateParam.UpdateCodeAddr;
	u32 beginBlock = offset/DATA_FLASH_SECTION_SIZE;
	u32 endBlock = (offset + size - 1)/DATA_FLASH_SECTION_SIZE;
	u32 blockNum = endBlock - beginBlock + 1;
	int i = 0;
	for(i = 0;i < blockNum;i++)
	{
		u32 blockSeq = beginBlock + i;
		//要先查看当前块是否已经执行过擦除
		if(CurrentUpdateParam.EraseBlockBitmap[blockSeq/8] & (1<<(blockSeq&7)))
		{
			continue;
		}
		else
		{
			EraseFlash(CurrentUpdateParam.UpdateCodeAddr + blockSeq*DATA_FLASH_SECTION_SIZE,DATA_FLASH_SECTION_SIZE);
			CurrentUpdateParam.EraseBlockBitmap[blockSeq/8] |= (1<<(blockSeq&7));
		}
	}
}

//接收完成态
void UpdateFinishRun(void *fsm,void* arg)
{
    if(TimeOut(&UpdateTimer))   //没收到执行升级命令,超时就直接升级然后重启
    {
    	debug_str(DEBUG_LOG_UPDATA, "UpdateFinishRun\r\n");
    	lcdiff_update(CurrentUpdateParam.UpdateCodeAddr, FLASH_CODE1_SIZE);
        FSMSystem *this = (FSMSystem *)fsm;

		debug_str(DEBUG_LOG_UPDATA, "UpdateFinishRun, timeout\r\n");
		FSMChangeState(this,UPDATE_STATE_IDLE);
		if(CurrentUpdateParam.IsOwnFile != 1) return ;
//#ifdef ZIP_UPDATA_SUPPORT
//		DecCode();
//#endif        

		DoUpdate(0);
        
        //重启
        REBOOT_SYSTEM();
    }
}

//升级进行态
void UpdateExecuteRun(void *fsm,void* arg)
{
    FSMSystem *this = (FSMSystem *)fsm;
    if(TimeOut(&UpdateTimer))       //到点复位
    {
    	debug_str(DEBUG_LOG_UPDATA, "UpdateExecuteRun\r\n");
    	lcdiff_update(CurrentUpdateParam.UpdateCodeAddr, FLASH_CODE1_SIZE);
    	debug_str(DEBUG_LOG_UPDATA, "UpdateExecuteRun, timeout\r\n");
		FSMChangeState(this,UPDATE_STATE_IDLE);
		if(CurrentUpdateParam.IsOwnFile != 1) return ;
//#ifdef ZIP_UPDATA_SUPPORT
//	    DecCode();
//#endif
        
		DoUpdate(0);
		REBOOT_SYSTEM();
    }
}

//开始升级处理报文
void StartUpdateEvent(void *fsm,void* arg)
{
    APP_CREATE_UPDATE_DEF();
    APP_FILE_INFO_DOWN *startUpdate = (APP_FILE_INFO_DOWN *)updatePacket->Data;
    FSMSystem *this = (FSMSystem *)fsm;
    u8 result = 0;
    memset(&CurrentUpdateParam,0,sizeof(CurrentUpdateParam));
	CurrentUpdateParam.IsOwnFile=0;
    CurrentUpdateParam.UpdateTimeout = startUpdate->UpdateWin * 60UL * 1000UL;
    CurrentUpdateParam.CurrentUpdateID = startUpdate->UpdateID;
    CurrentUpdateParam.CurrentTotalSegNum = startUpdate->FileSegment;
    CurrentUpdateParam.CurrentTotalFileSize = startUpdate->FileSize;
    CurrentUpdateParam.CurrentFileCRC = startUpdate->FileCRC;

    debug_str(DEBUG_LOG_UPDATA, "StartUpdateEvent, src:%d, dst:%d, FilePro:%d, time:%d min, size:%d, segment:%d\r\n", 
        macHead->SrcTei, macHead->DstTei,
        startUpdate->FilePro, startUpdate->UpdateWin, startUpdate->FileSize, startUpdate->FileSegment);
    
    if(startUpdate->FilePro != APP_FILE_CLEAR)
    {
        CurrentUpdateParam.FilePro = startUpdate->FilePro;
        if(startUpdate->FileSize > MAX_UPDATE_FILE_SIZE)
            result = 1;
        if(startUpdate->FileSegment < 1)
            result = 1;
		CurrentUpdateParam.IsOwnFile=0;
		CurrentUpdateParam.CurrentFileSize=0;
		memset(CurrentUpdateParam.EraseBlockBitmap,0,sizeof(CurrentUpdateParam.EraseBlockBitmap));
        if(result == 0)
		{
			CurrentUpdateParam.MaxBlockSeq    = CurrentUpdateParam.CurrentTotalSegNum - 1;
			CurrentUpdateParam.UpdateUserAddr = FLASH_USER_FEATURE1_ADDRESS + OldCodeSection * DATA_FLASH_SECTION_SIZE;
			CurrentUpdateParam.UpdateUserSize = FLASH_USER_FEATURE1_SIZE;
			CurrentUpdateParam.UpdateCodeAddr = FLASH_CODE1_ADDRESS + OldCodeSection * FLASH_CODE1_SIZE;
			CurrentUpdateParam.UpdataCodeSize = FLASH_CODE1_SIZE;
		}
    }
    
    if(appGeneral->Ctrl.AckFlg)
    {
        //组回应报文上行
        SEND_PDU_BLOCK* pdu = MallocSendPDU();
        if(pdu == NULL)
            return;
         //APP包头
        APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
        //业务包头
        APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
        //升级包头
        APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
        //上行帧
        APP_FILE_INFO_UP *sendAppFileUp = (APP_FILE_INFO_UP*)sendUpdatePacket->Data;
        
         //生成APP包头帧
        CreateAppPacket(sendAppPacket);
        //生成业务包头帧
        CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_FILE_INFO_UP),0);
        //生成升级包头
        CreateUpdateHeadFrame(sendUpdatePacket,APP_UPDATE_FILE_INFO);
        //生成上行帧
        CreateUpdateInfoFrame(sendAppFileUp,CurrentUpdateParam.CurrentUpdateID,result,0);
        
        pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
        MacHandleApp(pdu->pdu,pdu->size,2,CCO_TEI,SENDTYPE_SINGLE,BROADCAST_DRT_UP,0);
        FreeSendPDU(pdu);
    }
    if(result == 0 && CurrentUpdateParam.FilePro)
    {
        debug_str(DEBUG_LOG_UPDATA, "StartUpdateEvent, UPDATE_STATE_RECEIVING\r\n");
        FSMChangeState(this,UPDATE_STATE_RECEIVING);
    }
    else
    {
        debug_str(DEBUG_LOG_UPDATA, "StartUpdateEvent, UPDATE_STATE_IDLE\r\n");
        FSMChangeState(this,UPDATE_STATE_IDLE);
    }
    
    StartTimer(&UpdateTimer,CurrentUpdateParam.UpdateTimeout);
}
//预处理升级报文
bool StartFirstProc(void *fsm,void* arg)
{
	APP_CREATE_UPDATE_DEF();
	APP_FILE_INFO_DOWN *startUpdate = (APP_FILE_INFO_DOWN *)updatePacket->Data;

    debug_str(DEBUG_LOG_UPDATA, "StartFirstProc, state:%d, id:0x%x-0x%x\r\n", ((FSMSystem*)fsm)->CurrentState,
        startUpdate->UpdateID, CurrentUpdateParam.CurrentUpdateID);
    
	if (((FSMSystem*)fsm)->CurrentState == UPDATE_STATE_IDLE || startUpdate->UpdateID != CurrentUpdateParam.CurrentUpdateID)
	{
		StartUpdateEvent(fsm, arg);
		return true;
	}
	return false;
}
//处理开始升级重复报文
void StartUpdateRepeatEvent(void *fsm,void* arg)
{
#if 1
	APP_CREATE_UPDATE_DEF();
	APP_FILE_INFO_DOWN *startUpdate = (APP_FILE_INFO_DOWN *)updatePacket->Data;
    u8 result = 0;

    if(!appGeneral->Ctrl.AckFlg)
    {
        return;
    }
    
    if(CurrentUpdateParam.UpdateTimeout != startUpdate->UpdateWin * 60UL * 1000UL)
        result = 1;
    if(CurrentUpdateParam.CurrentUpdateID != startUpdate->UpdateID)
        result = 1;
    if(CurrentUpdateParam.CurrentTotalFileSize != startUpdate->FileSize)
        result = 1;
    if(CurrentUpdateParam.CurrentFileCRC != startUpdate->FileCRC)
        result = 1;
    
    //组回应报文上行
    SEND_PDU_BLOCK* pdu = MallocSendPDU();
    if(pdu == NULL)
        return;
    
    //APP包头
    APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
    //业务包头
    APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
    //升级包头
    APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
    //上行帧
    APP_FILE_INFO_UP *sendAppFileUp = (APP_FILE_INFO_UP*)sendUpdatePacket->Data;
    
    //生成APP包头帧
    CreateAppPacket(sendAppPacket);
    //生成业务包头帧
    CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_FILE_INFO_UP),0);
    //生成升级包头
    CreateUpdateHeadFrame(sendUpdatePacket,APP_UPDATE_FILE_INFO);
    //生成上行帧
    CreateUpdateInfoFrame(sendAppFileUp,CurrentUpdateParam.CurrentUpdateID,result,0);
	
    pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
    MacHandleApp(pdu->pdu,pdu->size,2,CCO_TEI,SENDTYPE_SINGLE,BROADCAST_DRT_UP,0);
    FreeSendPDU(pdu);        
#else
    APP_CREATE_UPDATE_DEF();
    APP_FILE_INFO_DOWN *startUpdate = (APP_FILE_INFO_DOWN *)updatePacket->Data;
    if(CurrentUpdateParam.CurrentUpdateID != startUpdate->UpdateID)
        StartUpdateEvent(fsm,arg);
#endif	
}

//在本地广播数据
void BroadcastFileData(void *fsm,void *arg)
{
   APP_CREATE_UPDATE_DEF();
   APP_FILE_DATA_DOWN *fileDown = (APP_FILE_DATA_DOWN*)updatePacket->Data;
    //转本地广播
    SEND_PDU_BLOCK* pdu = MallocSendPDU();
    if(pdu == NULL)
        return;
    
    //APP包头
    APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
    //业务包头
    APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
    //升级包头
    APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
    //数据帧
    APP_FILE_DATA_DOWN *sendAppFileUp = (APP_FILE_DATA_DOWN*)sendUpdatePacket->Data;
    
     //生成APP包头帧
    CreateAppPacket(sendAppPacket);
    //生成业务包头帧
    CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_FILE_DATA_UP),0);
    //生成升级包头
    CreateUpdateHeadFrame(sendUpdatePacket,APP_UPDATE_FILE_DATA);
    //生成数据帧
    memcpy(sendAppFileUp,fileDown,appGeneral->AppFrameLen - (sizeof(APP_UPDATE_PACKET) - 1));
    
    pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
    MacHandleApp(pdu->pdu,pdu->size,2,MAC_BROADCAST_TEI,SENDTYPE_LOCAL,BROADCAST_DRT_DOWN,0);
    FreeSendPDU(pdu);
}

//处理文件传输,单播
void UpdateHandleFileData(void *fsm,void* arg)
{
    APP_CREATE_UPDATE_DEF();
    APP_FILE_DATA_DOWN *fileDown = (APP_FILE_DATA_DOWN*)updatePacket->Data;
    FSMSystem *this = (FSMSystem *)fsm;
    //位图置位
    if(fileDown->UpdateID == CurrentUpdateParam.CurrentUpdateID)
    {
#ifdef BLE_NET_ENABLE
		if(lc_file_state_cbfun != NULL)
		{
			lc_file_state_cbfun(LC_FILE_WRITING);
		}
#endif
        u16 arrayindex = fileDown->FileSegID/8;
		u16 bitindex = fileDown->FileSegID%8;
        debug_str(DEBUG_LOG_UPDATA, "up get %d seq file\r\n", fileDown->FileSegID);
		if (fileDown->FileSegID>CurrentUpdateParam.CurrentTotalSegNum)
		{
			return;
		}
		if(((CurrentUpdateParam.FileBitmap[arrayindex] >> bitindex)&0x01) != 0x01)
		{
			if (fileDown->FileSegLen > CurrentUpdateParam.CurrentFileSegSize) CurrentUpdateParam.CurrentFileSegSize = fileDown->FileSegLen;
			u32 writeAddr = CurrentUpdateParam.CurrentFileSegSize * fileDown->FileSegID + CurrentUpdateParam.UpdateCodeAddr;
			CurrentUpdateParam.FileBitmap[arrayindex] |= (1 << bitindex);
			CurrentUpdateParam.FileBitmapSize = fileDown->FileSegID / 8 + 1;

			
			if (CurrentUpdateParam.CurrentTotalFileSize < FLASH_CODE1_SIZE) //大于code大小的只走流程 不写入flash  这不是自家code
			{

				if ((CurrentUpdateParam.UpdateCodeAddr==FLASH_CODE1_ADDRESS&&(writeAddr>=FLASH_CODE1_ADDRESS)&&writeAddr<(FLASH_CODE1_ADDRESS+FLASH_CODE1_SIZE))
					 || (CurrentUpdateParam.UpdateCodeAddr == FLASH_CODE2_ADDRESS && (writeAddr >= FLASH_CODE2_ADDRESS) && writeAddr < (FLASH_CODE2_ADDRESS + FLASH_CODE2_SIZE))
					#if defined(ZB205_CHIP)
					|| (CurrentUpdateParam.UpdateCodeAddr == FLASH_CODE3_ADDRESS && (writeAddr >= FLASH_CODE3_ADDRESS) && writeAddr < (FLASH_CODE3_ADDRESS + FLASH_CODE3_SIZE))
					#endif
					)
				{
					DataFlashInit();
					Unprotection1MFlash();
					EraseUpdateBlock(writeAddr, fileDown->FileSegLen);
					WriteFlash(writeAddr, fileDown->Data, fileDown->FileSegLen);
					Protection1MFlash();
					DataFlashClose();
				}
			}


			CurrentUpdateParam.CurrentFileSize += fileDown->FileSegLen;

			//接收完成
			if (CurrentUpdateParam.CurrentFileSize >= CurrentUpdateParam.CurrentTotalFileSize)
			{
#ifndef GW_TEST_VERSION
				//todoCRC校验
				AplCommonParam_type param;
				ReadFlash(CurrentUpdateParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE, (u8 *)&param, sizeof(AplCommonParam_type));

				//判断是否自家模块
				if (param.FactoryCode == 
#if defined(I_STA)
					FACTORY_CODE_I_FLAG						
#elif defined(II_STA)		
					FACTORY_CODE_II_FLAG
#else
					FACTORY_CODE_FLAG
#endif				
					&&CurrentUpdateParam.CurrentTotalFileSize < FLASH_CODE1_SIZE)
				{
					
					//计算前CRC
					if (CommonGetCrc32((u8 *)CurrentUpdateParam.UpdateCodeAddr, FLASH_INTERRUPT_SIZE) == param.FontFileCrc32)
					{
						if (param.CodeLength < FLASH_CODE1_SIZE)
						{
							//计算后CRC
							if (CommonGetCrc32((u8 *)(CurrentUpdateParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE + 4), param.CodeLength - FLASH_INTERRUPT_SIZE - 4) == param.RearFileCrc32)
							{
							    debug_str(DEBUG_LOG_UPDATA, "finish recv total update file\r\n");
								CurrentUpdateParam.IsOwnFile=1;
								//CRC校验通过,转换成接收完成态
								FSMChangeState(this, UPDATE_STATE_FINISH);
								return;
							}
						}
					}
					else if(lcdiff_update(CurrentUpdateParam.UpdateCodeAddr, FLASH_CODE1_SIZE) == 1)
					{
						debug_str(DEBUG_LOG_UPDATA, "finish recv total update file\r\n");
					}
					else
					{
						last_up_status=0x2;
					}
				}
                else
                {
                    #ifdef BLE_NET_ENABLE
                        if((lc_app_file_check_cbfun != NULL)
                            &&(lc_file_state_cbfun != NULL))
                        {
                            if(lc_app_file_check_cbfun())
                            {
                                lc_file_state_cbfun(LC_FILE_FINISH);
                            }
                        }
                    #endif
#ifndef II_STA
                    if(*(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_FLAG)
#else
                    if(*(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_II_FLAG)
#endif
                    {
                        debug_str(DEBUG_LOG_UPDATA, "finish recv total lzip update file\r\n");
                    }
                }     
				FSMChangeState(this, UPDATE_STATE_FINISH);
#else
				FSMChangeState(this, UPDATE_STATE_FINISH);
#endif

			}
		}
	}
    if(appGeneral->Ctrl.AckFlg)
    {
        //组回应报文上行
        SEND_PDU_BLOCK* pdu = MallocSendPDU();
        if(pdu == NULL)
            return;
         //APP包头
        APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
        //业务包头
        APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
        //升级包头
        APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
        //上行帧
        APP_FILE_DATA_UP *sendAppFileUp = (APP_FILE_DATA_UP*)sendUpdatePacket->Data;
        
         //生成APP包头帧
        CreateAppPacket(sendAppPacket);
        //生成业务包头帧
        CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_FILE_DATA_UP),0);
        //生成升级包头
        CreateUpdateHeadFrame(sendUpdatePacket,updatePacket->UpdateInfoID);

        //生成上行帧
        sendAppFileUp->UpdateID = CurrentUpdateParam.CurrentUpdateID;
        sendAppFileUp->ResultCode = 0;
        
        pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
        MacHandleApp(pdu->pdu,pdu->size,2,CCO_TEI,SENDTYPE_SINGLE,BROADCAST_DRT_UP,0);
        FreeSendPDU(pdu);
    }
}

//处理文件传输,单播转广播
void UpdateHandleFileDataBroadcast(void *fsm,void* arg)
{
    UpdateHandleFileData(fsm,arg);
     //广播出去
    BroadcastFileData(fsm,arg);
}

//查询升级状态
void UpdateQueryState(u8* arg,u16 len)
{
    APP_CREATE_UPDATE_DEF();
    APP_REC_STATE_DOWN *stateDown = (APP_REC_STATE_DOWN*)updatePacket->Data;
    if(appGeneral->Ctrl.AckFlg)
    {
        //组回应报文上行
        SEND_PDU_BLOCK* pdu = MallocSendPDU();
        if(pdu == NULL)
            return;
         //APP包头
        APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
        //业务包头
        APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
        //升级包头
        APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
        //上行帧
        APP_REC_STATE_UP *sendAppStateUp = (APP_REC_STATE_UP*)sendUpdatePacket->Data;
        
         //生成APP包头帧
        CreateAppPacket(sendAppPacket);
        //生成业务包头帧
        CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1,0);
        //生成升级包头
        CreateUpdateHeadFrame(sendUpdatePacket,updatePacket->UpdateInfoID);
        sendAppStateUp->UpdateID = stateDown->UpdateID;
        sendAppStateUp->BeginSegment = stateDown->BeginSegment;
		sendAppStateUp->Reserve = 0;
        int currentState = FSMGetState(&UpdateFSM);
        if(currentState < UPDATE_STATE_FINISH)
        {
            sendAppStateUp->State = 0x01;
        }
        else
        {
            if(CurrentUpdateParam.FilePro == APP_FILE_STA)
                sendAppStateUp->State = 0x02;
            else
                sendAppStateUp->State = 0x05;
        }
        u16 bitmapSize = 0;
        if(GetUpdateBlockNum(stateDown->BeginSegment,stateDown->SegmentNum,stateDown->UpdateID,sendAppStateUp->Data,&bitmapSize))
        {
        	//存在无效数据块
        	sendAppStateUp->State = 0x01;
        }
        sendAppGeneralPacket->AppFrameLen = sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_REC_STATE_UP) - 1 + bitmapSize;
        
        pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
        MacHandleApp(pdu->pdu,pdu->size,2,CCO_TEI,SENDTYPE_SINGLE,BROADCAST_DRT_UP,0);
        FreeSendPDU(pdu);
    }
    
}
void UpdateQueryStateReader(u8* arg,u16 len)
{
    APP_CREATE_UPDATE_DEF();
    APP_REC_STATE_READER_DOWN *stateDown = (APP_REC_STATE_READER_DOWN*)updatePacket->Data;
	const ST_STA_ATTR_TYPE *pNetBasePara = GetStaBaseAttr();
	TransposeAddr(stateDown->mac);
	if (memcmp(pNetBasePara->Mac, stateDown->mac,sizeof(pNetBasePara->Mac))!=0)
	{
		return;
	}
	if (appGeneral->Ctrl.AckFlg)
    {
        //组回应报文上行
        SEND_PDU_BLOCK* pdu = MallocSendPDU();
        if(pdu == NULL)
            return;
         //APP包头
        APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
        //业务包头
        APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
        //升级包头
        APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
        //上行帧
        APP_REC_STATE_READER_UP *sendAppStateUp = (APP_REC_STATE_READER_UP*)sendUpdatePacket->Data;
        
         //生成APP包头帧
        CreateAppPacket(sendAppPacket);
        //生成业务包头帧
        CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1,0);
        //生成升级包头
        CreateUpdateHeadFrame(sendUpdatePacket,updatePacket->UpdateInfoID);
        sendAppStateUp->UpdateID = stateDown->UpdateID;
        sendAppStateUp->BeginSegment = stateDown->BeginSegment;
		sendAppStateUp->Reserve = 0;
		memcpy(sendAppStateUp->mac, pNetBasePara->Mac,sizeof(sendAppStateUp->mac));
		TransposeAddr(sendAppStateUp->mac);
        int currentState = FSMGetState(&UpdateFSM);
        if(currentState < UPDATE_STATE_FINISH)
        {
            sendAppStateUp->State = 0x01;
        }
        else
        {
            if(CurrentUpdateParam.FilePro == APP_FILE_STA)
                sendAppStateUp->State = 0x02;
            else
                sendAppStateUp->State = 0x05;
        }
        u16 bitmapSize = 0;
        if(GetUpdateBlockNum(stateDown->BeginSegment,stateDown->SegmentNum,stateDown->UpdateID,sendAppStateUp->Data,&bitmapSize))
        {
        	//存在无效数据块
        	sendAppStateUp->State = 0x01;
        }
        sendAppGeneralPacket->AppFrameLen = sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_REC_STATE_READER_UP) - 1 + bitmapSize;
        
        pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
        MacHandleApp(pdu->pdu,pdu->size,2,READER_TEI,SENDTYPE_SINGLE,BROADCAST_DRT_UP,0);
        FreeSendPDU(pdu);
    }
    
}

//执行升级
void UpdateExecuteEvent(void *fsm,void* arg)
{
    APP_CREATE_UPDATE_DEF();
    
    APP_FILE_FINISH_DOWN *updateDown = (APP_FILE_FINISH_DOWN*)updatePacket->Data;
    //FSMSystem *this = (FSMSystem *)fsm;
    
    if(updateDown->UpdateID == CurrentUpdateParam.CurrentUpdateID)
    {
    	debug_str(DEBUG_LOG_UPDATA, "UpdateExecuteEvent");
    	lcdiff_update(CurrentUpdateParam.UpdateCodeAddr, FLASH_CODE1_SIZE);
		if(CurrentUpdateParam.IsOwnFile != 1) return ;
		DoUpdate(0);
		
        DebugUpdateInfo.DebugCRC = CurrentUpdateParam.CurrentFileCRC;
        DebugUpdateInfo.DebugFileSize = CurrentUpdateParam.CurrentFileSize;
        DebugUpdateInfo.DebugSoftVersion = DEFAULT_SOFT_VERSION + 1;

        debug_str(DEBUG_LOG_UPDATA, "UpdateExecuteEvent, UpdateTimer:%d sec\r\n", updateDown->DelayTime);
        
        if(updateDown->DelayTime*1000 > FIRST_READ_ADDR_DELAY)
            StartTimer(&UpdateTimer,updateDown->DelayTime*1000UL - FIRST_READ_ADDR_DELAY);
        else
        {
            StartTimer(&UpdateTimer,1);
        }
		if(appGeneral->Ctrl.AckFlg)
		{
			//组回应报文上行
			SEND_PDU_BLOCK* pdu = MallocSendPDU();
			if(pdu == NULL)
				return;
			 //APP包头
			APP_PACKET* sendAppPacket = (APP_PACKET*)pdu->pdu;
			//业务包头
			APP_GENERAL_PACKET *sendAppGeneralPacket = (APP_GENERAL_PACKET*)&sendAppPacket->Data[0];
			//升级包头
			APP_UPDATE_PACKET *sendUpdatePacket = (APP_UPDATE_PACKET*)sendAppGeneralPacket->Data;
			//上行帧
			APP_FILE_DATA_UP *sendAppFileUp = (APP_FILE_DATA_UP*)sendUpdatePacket->Data;
			
			 //生成APP包头帧
			CreateAppPacket(sendAppPacket);
			//生成业务包头帧
			CreateAppBusinessPacket(sendAppGeneralPacket,APP_FRAME_CMD,0,0,0,1,APP_BUS_FILE_TRAN,appGeneral->AppSeq,sizeof(APP_UPDATE_PACKET) - 1 + sizeof(APP_FILE_DATA_UP),0);
			//生成升级包头
			CreateUpdateHeadFrame(sendUpdatePacket,updatePacket->UpdateInfoID);

			//生成上行帧
			sendAppFileUp->UpdateID = CurrentUpdateParam.CurrentUpdateID;
			sendAppFileUp->ResultCode = 0;
			
			pdu->size = sizeof(APP_PACKET) - 1 + sizeof(APP_GENERAL_PACKET) - 1 + sendAppGeneralPacket->AppFrameLen;
			MacHandleApp(pdu->pdu,pdu->size,2,CCO_TEI,SENDTYPE_SINGLE,BROADCAST_DRT_UP,0);
			FreeSendPDU(pdu);
		}
    }
}

void UpdateFSMRun(void* arg)
{
    FSMSystemRun(&UpdateFSM);
}

void UpdateRebootSystem(void *arg)
{
    REBOOT_SYSTEM();
}

//初始化
void UpdateInit(u32 tryRunTime)
{
    FSMSystemInit(&UpdateFSM);
    //注册状态动作
    FSMSystemRegisterState(&UpdateFSM,UPDATE_STATE_IDLE,UpdateIdleRun);
    FSMSystemRegisterState(&UpdateFSM,UPDATE_STATE_RECEIVING,UpdateReceivingRun);
    FSMSystemRegisterState(&UpdateFSM,UPDATE_STATE_FINISH,UpdateFinishRun);
    FSMSystemRegisterState(&UpdateFSM,UPDATE_STATE_EXECUTE,UpdateExecuteRun);
    
    //注册触发动作
    
    //空闲->开始升级->原来(在里面判断)
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_IDLE,UPDATE_EVENT_START,INVAILD_STATE,StartUpdateEvent);
    //空闲->单播转本地广播->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_IDLE,UPDATE_EVENT_FILE_DATA_BROADCAST,INVAILD_STATE,BroadcastFileData);
    //接收进行态->文件数据->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_RECEIVING,UPDATE_EVENT_FILE_DATA,INVAILD_STATE,UpdateHandleFileData);
    //接收进行态->单播转本地广播->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_RECEIVING,UPDATE_EVENT_FILE_DATA_BROADCAST,INVAILD_STATE,UpdateHandleFileDataBroadcast);
    //接收完成态->执行升级->升级进行态
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_FINISH,UPDATE_EVENT_EXECUTE,UPDATE_STATE_EXECUTE,UpdateExecuteEvent);
    //接收完成态->单播转本地广播->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_FINISH,UPDATE_EVENT_FILE_DATA_BROADCAST,INVAILD_STATE,BroadcastFileData);
    
    //注册开始升级触发动作
    //接收进行态->开始升级->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_RECEIVING,UPDATE_EVENT_START,INVAILD_STATE,StartUpdateRepeatEvent);
    //接收完成态->开始升级->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_FINISH,UPDATE_EVENT_START,INVAILD_STATE,StartUpdateRepeatEvent);
    //升级进行态->开始升级->原来
    FSMSystemRegistetEvent(&UpdateFSM,UPDATE_STATE_EXECUTE,UPDATE_EVENT_START,INVAILD_STATE,StartUpdateRepeatEvent);
    
    
    memset(&CurrentUpdateParam,0,sizeof(CurrentUpdateParam));
    
    //初始状态为空闲
    FSMChangeState(&UpdateFSM,UPDATE_STATE_IDLE);
    
    //状态机运行(目前使用定时器做)
    HPLC_RegisterTmr(UpdateFSMRun,0,1,TMR_OPT_CALL_PERIOD);
    
    //初始化调试数据
    InitDebugUpdateInfo();
    
    StartTimer(&UpdateTimer,1);

#ifdef BLE_NET_ENABLE
			if(lc_file_state_cbfun != NULL)
			{
				lc_file_state_cbfun(LC_FILE_UNKNOW);
			}
#endif

}


void UpdateReceiveEventData(u8* data,u16 len)
{
    MAC_HEAD *macHead = (MAC_HEAD*)data;
    APP_PACKET *appPacket = 0;
    APP_GENERAL_PACKET *appGeneral = 0;

    if(macHead->HeadType)
        appPacket = (APP_PACKET*)((u8*)macHead + sizeof(MAC_HEAD_SHORT) + sizeof(MSDU_HEAD_SHORT));
    else
        appPacket = (APP_PACKET*)((u8*)macHead + sizeof(MAC_HEAD_LONG) + sizeof(MSDU_HEAD_LONG));

    appGeneral = (APP_GENERAL_PACKET*)&appPacket->Data[0];
    if (appGeneral->Ctrl.TranDir == 1) //上行
		return;
    
    APP_UPDATE_PACKET *updatePacket = (APP_UPDATE_PACKET*)&appGeneral->Data[0];
    
    debug_str(DEBUG_LOG_UPDATA, "UpdateReceiveEventData, UpdateInfoID:%d\r\n", updatePacket->UpdateInfoID);
    
    switch(updatePacket->UpdateInfoID)
    {
        case APP_UPDATE_FILE_INFO:     //下发文件信息
        {
            FSMSystemStateChange(&UpdateFSM,UPDATE_EVENT_START,data);
            break;
        }
        case APP_UPDATE_FILE_DATA:       //下发文件数据
        {
            FSMSystemStateChange(&UpdateFSM,UPDATE_EVENT_FILE_DATA,data);
            break;
        }
        case APP_UPDATE_STATE:      //升级接收包状态
        {
            UpdateQueryState(data,len);
            break;
        }
		case APP_UPDATE_STATE_READER:      //升级接收包状态（抄控器）
        {
            UpdateQueryStateReader(data,len);
            break;
        }
        case APP_UPDATE_FINISH:     //文件传输完成通知
        {
            FSMSystemStateChange(&UpdateFSM,UPDATE_EVENT_EXECUTE,data);
            break;
        }
        case APP_UPDATE_BROADCAST:   //本地广播转发
        {
            FSMSystemStateChange(&UpdateFSM,UPDATE_EVENT_FILE_DATA_BROADCAST,data);
            break;
        }
        default:break;
    }
}

//升级状态
u8 GetUpdateState(void)
{
    int state = FSMGetState(&UpdateFSM);
    switch(state)
    {
        case UPDATE_STATE_INVAILD://无效
        case UPDATE_STATE_IDLE:        //空闲
            return 0;
            break;
        case UPDATE_STATE_RECEIVING:   //接收进行态
            return 1;
            break;
        case UPDATE_STATE_FINISH:      //接收完成态
            return 2;
            break;
        case UPDATE_STATE_EXECUTE:     //升级进行态
            return 3;
            break;
            break;
        default:return 0;
    }
}

//有效块数（起始块号，连续查询的块数，位图，位图大小）
u32 GetUpdateBlockNum(u32 beginBlock,u32 blockNum,u32 updateID,u8 *bitmap,u16 *bitmapSize)
{
    u32 blockSeq = 0;
    u32 blockQueryNum = 0;  //查到哪一块
    u32 blockTotalNum = 0;
	u8 flag = 0; //0-全部有效，1-存在无效块数
    if(updateID != CurrentUpdateParam.CurrentUpdateID)
    {
        *bitmapSize = 0;
        return 0;
    }
    if(blockNum == 0)
    {
        *bitmapSize = 0;
        return 0;
    }
    if(blockNum == 0xFFFF)
        blockQueryNum = CurrentUpdateParam.MaxBlockSeq;
    else
    {
        blockQueryNum = beginBlock + blockNum - 1;
        if(blockQueryNum > CurrentUpdateParam.MaxBlockSeq)
		{
            blockQueryNum = CurrentUpdateParam.MaxBlockSeq;
		}
    }
	memset(bitmap,0,(blockQueryNum+7)/8);
    for(blockSeq = beginBlock;blockSeq <= blockQueryNum;blockSeq++)
    {
        if(CurrentUpdateParam.FileBitmap[blockSeq/8] & (1<<(blockSeq&7)))
        {
            //bitmap[blockTotalNum/8] |= (1<<(blockTotalNum&7));
            //南网状态位与国外相反
            bitmap[blockTotalNum/8] |= (1<<(7-(blockTotalNum&7)));
        }
#ifdef PROTOCOL_NW_2020		
		else
		{
			flag = 1;
		}
#endif		
        blockTotalNum++;
    }

#ifdef PROTOCOL_NW_2020	
	if(flag == 0) //全部有效
	{
		*bitmapSize = 0;
		memset(bitmap, 0, (blockQueryNum+7)/8);
	}
	else
#endif		
	{
    	*bitmapSize = (blockTotalNum+7)/8;
	}
	
    return flag;
}

//升级ID
u32 GetUpdateID(void)
{
	return CurrentUpdateParam.CurrentUpdateID;
}

//升级文件CRC
u32 GetUpdateFileCRC(void)
{
#ifndef GW_TEST_VERSION
	return CurrentTryRunParam.FileCRC;
#else
	return DebugUpdateInfo.DebugCRC;
#endif
}

//升级文件大小
u32 GetUpdateFileSize(void)
{
#ifndef GW_TEST_VERSION
	return CurrentTryRunParam.FileSize;
#else
	return DebugUpdateInfo.DebugFileSize;
#endif
}

#ifdef ZIP_UPDATA_SUPPORT
/**********************************************************
压缩文件处理步骤
**********************************************************/
#pragma pack(4)
#include "LzmaLib.h"
#pragma segment=".data"
#define MY_ALLOC_MEM_SIZE   (128*1024)//128K

void DecCode(void)
{
  //校验文件正确性 以及是否是自家芯片
  DEC_FILE_HEAD *fHead = (DEC_FILE_HEAD*)CurrentUpdateParam.UpdateCodeAddr;
  if (
#if defined(I_STA)
	  *(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_I_FLAG&&
#elif defined(II_STA)
	  *(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_II_FLAG&&
#else
	  *(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_FLAG &&
#endif
	  CurrentUpdateParam.CurrentTotalFileSize<FLASH_CODE1_SIZE
	  )
  {
	  if (CommonGetCrc32(fHead->data, fHead->fileSize) == fHead->crc) //校验CRC
	  {
		  //此处不可有系统函数的调用  因为系统函数会调用全局变量  破坏解压缩数据
		  __disable_fiq();
		  //关闭HPLC 放止由于接收到hplc报文后dma复写ram破坏原本的解压缩数据
		  BPLC_Reset();
		  BPLC_Stop();
		  HRF_Reset();
		  HRF_Stop();
		  //设置栈为系统默认栈帧位置
		  u32 stackhead = (u32)__sfb(".data") + DEC_STACK_SIZE;
		  stackhead &= 0xfffffff8; //8字节对齐
		  __set_PSP(stackhead);
		  __set_MSP(stackhead);
		  //大括号是必须的重新设置栈帧后临时变量需要重新配置
		  {
			  UserSection_Type* remCurrentCodeSection=CurrentCodeSection;//保存CurrentCodeSection
			  u32 stackhead = (u32)__sfb(".data") + DEC_STACK_SIZE;
			  stackhead &= 0xfffffff8; //8字节对齐
			  unsigned char *allocPtr = (unsigned char *)stackhead;
			  DEC_FILE_HEAD *fHead = (DEC_FILE_HEAD *)CurrentUpdateParam.UpdateCodeAddr;
			  allocPtr += 8; //用于解压缩的申请内存
			  u8 *decAddr = allocPtr + MY_ALLOC_MEM_SIZE; //解压缩的暂存位置
			  UpdateParam copyUpParam;
			  memcpy(&copyUpParam, &CurrentUpdateParam, sizeof(copyUpParam)); //获得升级文件信息
			  u8 props[5];
			  memcpy(props, fHead->props, sizeof(props));
			  size_t destLen = fHead->decFileSize;
			  SizeT srcLen = fHead->fileSize;
			  FeedWdg();
			  if (destLen > FLASH_CODE1_SIZE)
			  {
				  RebootSystem();
			  }
			  if(LzmaUncompress(decAddr, &destLen, fHead->data, &srcLen, props, 5))
			  {
				  //解压错误
				  RebootSystem();
			  }
			  FeedWdg();
			  u32 upLimitSize = (destLen + 4095) / 4096 * 4096; //对4K上溢对齐
			  u32 flashStart = copyUpParam.UpdateCodeAddr;
			  //此处flash操作不可使用内部调用系统函数的WriteFlash,EraseFlash等函数
			  FLASH_Init();
			  Unprotection1MFlash();
			  for (int i = 0; i < upLimitSize; i += 4096)
			  {
				  EraseDataFlash(flashStart + i,4096);
				  WriteDataFlash(flashStart + i, decAddr + i, 4096);
				  FeedWdg();
			  }
			  Protection1MFlash();
			  FLASH_ResetAtFault();
			  FLASH_Close();
			  //写完code后可以使用系统函数了
			  //
			  //校验文件是否写入flash正确
			  AplCommonParam_type param;
			  ReadFlash(copyUpParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE, (u8 *)&param, sizeof(AplCommonParam_type));

			  //计算前CRC
			  if (CommonGetCrc32((u8 *)copyUpParam.UpdateCodeAddr, FLASH_INTERRUPT_SIZE) != param.FontFileCrc32)
			  {
				  RebootSystem();
			  }
			  if (param.CodeLength < FLASH_CODE1_SIZE)
			  {
				  //计算后CRC
				  if (CommonGetCrc32((u8 *)(copyUpParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE + 4), param.CodeLength - FLASH_INTERRUPT_SIZE - 4) != param.RearFileCrc32)
				  {
					  RebootSystem();
				  }
			  }
			  else
			  {
				  RebootSystem();
			  }
			  FeedWdg();
			  copyUpParam.IsOwnFile = 1;
			  //copyUpParam.CurrentTotalFileSize = destLen;
			  CurrentCodeSection=remCurrentCodeSection;
			  memcpy( &CurrentUpdateParam,&copyUpParam, sizeof(copyUpParam)); 
			  BootPowerState = 1;
			  DoUpdate(0);
			  //等待重启
			  RebootSystem();
		  }
	  }
  }
  
}
#endif

// #define BLE_NET_ENABLE
#ifdef BLE_NET_ENABLE
bool LocalStartUpdataFileProcess(uint32_t id,uint32_t size,uint32_t crc,uint32_t type,uint32_t seg_cnt)
{	
    //bool result = false;
	if(type >= 2)return false;
	if(type == 0)
	{
		memset(&CurrentUpdateParam,0,sizeof(CurrentUpdateParam));
		return true;
	}

    if(((seg_cnt/8) > MAX_UPDATEMAPBITSIZE))return false;
	if(size > MAX_UPDATE_FILE_SIZE) return false;
	//uint32_t package_size = (size + seg_cnt - 1)/seg_cnt;
	//if((package_size&0x1F) != 0)return false;
	
    CurrentUpdateParam.CurrentFileSegSize = 0;
    CurrentUpdateParam.UpdateTimeout = 90 * 60UL * 1000UL;
    CurrentUpdateParam.CurrentUpdateID = id;
    CurrentUpdateParam.CurrentTotalFileSize = size;
    CurrentUpdateParam.CurrentFileCRC = crc;
	CurrentUpdateParam.MaxBlockSeq = seg_cnt-1;
	CurrentUpdateParam.CurrentTotalSegNum = seg_cnt;

    CurrentUpdateParam.UpdateUserAddr = FLASH_USER_FEATURE1_ADDRESS + OldCodeSection * DATA_FLASH_SECTION_SIZE;
    CurrentUpdateParam.UpdateUserSize = FLASH_USER_FEATURE1_SIZE;
    CurrentUpdateParam.UpdateCodeAddr = FLASH_CODE1_ADDRESS + OldCodeSection * FLASH_CODE1_SIZE;
    CurrentUpdateParam.UpdataCodeSize = FLASH_CODE1_SIZE;
	return true;
}

__WEAK void StartUpdateFinishTimerToResetSystem(u32 willResetInMs)
{
	REBOOT_SYSTEM();
}
bool LocalUpdataFileSegmetProcess(uint32_t seg_offset,uint32_t seg_crc,uint8_t *seg_data,uint32_t seg_len)
{

	bool rflg = false;
	if((seg_offset + 1) > CurrentUpdateParam.CurrentTotalSegNum)return false;
	u16 arrayindex = seg_offset/8;
	u16 bitindex = seg_offset%8;
	if(((CurrentUpdateParam.FileBitmap[arrayindex] >> bitindex)&0x01) != 0x01)
	{
		//u32 writeAddr = CurrentUpdateParam.CurrentBlockSize*fileDown->BlockSeq + CurrentUpdateParam.UpdateCodeAddr;
		if(CurrentUpdateParam.CurrentFileSegSize == 0)
		{
			CurrentUpdateParam.CurrentFileSegSize = seg_len;
		}
		if((seg_offset+2) < CurrentUpdateParam.CurrentTotalSegNum)
		{
			if(CurrentUpdateParam.CurrentFileSegSize != seg_len)return false;
		}
		if(CurrentUpdateParam.CurrentFileSegSize == 0)return false;
		u32 writeAddr = (CurrentUpdateParam.CurrentFileSegSize)*seg_offset + CurrentUpdateParam.UpdateCodeAddr;
		CurrentUpdateParam.FileBitmap[arrayindex] |= (1<<bitindex);
		CurrentUpdateParam.FileBitmapSize = seg_offset/8 + 1;
		
        if (CurrentUpdateParam.CurrentTotalFileSize < FLASH_CODE1_SIZE) //大于code大小的只走流程 不写入flash  这不是自家code
        {

            if ((CurrentUpdateParam.UpdateCodeAddr==FLASH_CODE1_ADDRESS&&(writeAddr>=FLASH_CODE1_ADDRESS)&&writeAddr<(FLASH_CODE1_ADDRESS+FLASH_CODE1_SIZE))
                    || (CurrentUpdateParam.UpdateCodeAddr == FLASH_CODE2_ADDRESS && (writeAddr >= FLASH_CODE2_ADDRESS) && writeAddr < (FLASH_CODE2_ADDRESS + FLASH_CODE2_SIZE))
                #if defined(ZB205_CHIP)
                || (CurrentUpdateParam.UpdateCodeAddr == FLASH_CODE3_ADDRESS && (writeAddr >= FLASH_CODE3_ADDRESS) && writeAddr < (FLASH_CODE3_ADDRESS + FLASH_CODE3_SIZE))
                #endif
                )
            {
                DataFlashInit();
                Unprotection1MFlash();
                EraseUpdateBlock(writeAddr, seg_len);
                WriteFlash(writeAddr, seg_data, seg_len);
                Protection1MFlash();
                DataFlashClose();
            }
        }
		
		CurrentUpdateParam.CurrentFileSize += seg_len;
		// APP_PRINTF("update recv: %d | %d\r\n", seg_offset, seg_len);
		
	}
	rflg = true;
	//接收完成
	if(CurrentUpdateParam.CurrentFileSize >= CurrentUpdateParam.CurrentTotalFileSize)
	{
#ifndef GW_TEST_VERSION
				//todoCRC校验
				AplCommonParam_type param;
				ReadFlash(CurrentUpdateParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE, (u8 *)&param, sizeof(AplCommonParam_type));

				//判断是否自家模块
				if (param.FactoryCode == 
#if defined(I_STA)
					FACTORY_CODE_I_FLAG						
#elif defined(II_STA)		
					FACTORY_CODE_II_FLAG
#else
					FACTORY_CODE_FLAG
#endif				
					&&CurrentUpdateParam.CurrentTotalFileSize < FLASH_CODE1_SIZE)
				{
					
					//计算前CRC
					if (CommonGetCrc32((u8 *)CurrentUpdateParam.UpdateCodeAddr, FLASH_INTERRUPT_SIZE) == param.FontFileCrc32)
					{
						if (param.CodeLength < FLASH_CODE1_SIZE)
						{
							//计算后CRC
							if (CommonGetCrc32((u8 *)(CurrentUpdateParam.UpdateCodeAddr + FLASH_INTERRUPT_SIZE + 4), param.CodeLength - FLASH_INTERRUPT_SIZE - 4) == param.RearFileCrc32)
							{
							    debug_str(DEBUG_LOG_UPDATA, "finish recv total update file\r\n");
								CurrentUpdateParam.IsOwnFile=1;
								//CRC校验通过,转换成接收完成态
								// FSMChangeState(this, UPDATE_STATE_FINISH);
                                DoUpdate(0);

                                //重启
                                /*
                                OS_ERR err;
                                OSTimeDly(5*1000, OS_OPT_TIME_DLY, &err);
                                REBOOT_SYSTEM();
								*/
								StartUpdateFinishTimerToResetSystem(5000);
								return rflg;
							}
						}
					}
					// else
					{
						last_up_status=0x2;
					}
				}
				else if(lcdiff_update(CurrentUpdateParam.UpdateCodeAddr, FLASH_CODE1_SIZE) == 1)
				{
					StartUpdateFinishTimerToResetSystem(5000);
					return rflg;
				}	
                else
                {

#ifndef II_STA
                    if(*(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_FLAG)
#else
                    
                    if(*(u32*)CurrentUpdateParam.UpdateCodeAddr == FACTORY_CODE_II_FLAG)
#endif
                    {
                        debug_str(DEBUG_LOG_UPDATA, "finish recv total lzip update file\r\n");
                    }
                }
				// FSMChangeState(this, UPDATE_STATE_FINISH);
#else
				// FSMChangeState(this, UPDATE_STATE_FINISH);
#endif
	}
	return rflg;
}

uint32_t GetUpdateFileAddr(void)
{
    return CurrentUpdateParam.UpdateCodeAddr;
}
#endif
