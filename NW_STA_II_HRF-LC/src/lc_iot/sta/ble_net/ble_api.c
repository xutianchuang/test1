#include "ble_api.h"
#include "lc_all.h"
#include "ble_data.h"
#include "crclib.h"
#include "cpu.h"

const ST_STA_ID_TYPE *pStaChipID = &StaChipID;
const ST_STA_FIX_PARA *pStaFixPara = &StaFixPara;

//BLE内存池
OS_MEM Blepool;
static Ble_Mem_Union Blepdu[MAX_RECEIVE_DATA_NUM];

//===================================
ST_MODE_ID_INFO_TYPE IdInfo;

//===================================
all_ble_breakers_addr_t all_ble_breakers_addr;

//===================================
uint32_t GetCurrentHplcNetFreq()
{
	return pStaFixPara->baud;
}

//===================================
uint32_t lc_net_get_factory_id_code()
{
	return IdInfo.isp_key;
	// return ble_system_params.keyInfo.isp_key;
}

uint32_t lc_net_get_project_id_code()
{
	return IdInfo.proj_key;
	// return ble_system_params.keyInfo.proj_key;
}
extern void LcSetFactoryMsg(void);
bool lc_net_write_project_id_code(uint32_t key)
{
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	if(key != (IdInfo.proj_key))
	{
		IdInfo.proj_key = key;
		ble_key_info_stored();
		//重新入网
		if(pNetBasePara->NetState == NET_IN_NET)
		{
			OfflineSetState();
		}
		//设置key后需要通过api函数给底下同步
		LcSetFactoryMsg();
	}
	return true;
	// const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	// if(key != (ble_system_params.keyInfo.proj_key))
	// {
	// 	ble_system_params.keyInfo.proj_key = key;
	// 	ble_system_params.DirtySystemParamFlg = 1;
	// 	ble_system_info_stored();
	// 	//重新入网
	// 	if(pNetBasePara->NetState == NET_IN_NET)
	// 	{
	// 		OfflineSetState();
	// 	}
	// 	//
	// }
	// return true;
}

uint32_t lc_net_get_ex_param_code()
{
	return IdInfo.ex_param;
	// return ble_system_params.keyInfo.ex_param;
}

uint8_t lc_net_get_key_switch_code()
{
	return ble_system_params.key_switch;
}
bool lc_net_write_ex_param_code(uint32_t ex_param)
{
	ble_read_key_info();
	u32 ex_paramTemp = IdInfo.ex_param;
	if(ex_paramTemp != ex_param)
	{
        IdInfo.ex_param = ex_param;
		ble_key_info_stored();
	}
	return true;
	// u32 ex_paramTemp = ble_system_params.keyInfo.ex_param;
	// if(ex_paramTemp != ex_param)
	// {
    //     ble_system_params.keyInfo.ex_param = ex_param;
	// 	ble_system_params.DirtySystemParamFlg = 1;
	// 	ble_system_info_stored();
	// }
	// return true;
}

bool lc_net_write_factory_id_code(uint32_t key)
{
	if(key != (IdInfo.isp_key))
	{
		// CPU_SR_ALLOC();
		// OS_CRITICAL_ENTER();
		IdInfo.isp_key = key;
		ble_key_info_stored();
		// CPU_CRITICAL_EXIT();
	}
	return true;
	// if(key != (ble_system_params.keyInfo.isp_key))
	// {
	// 	// CPU_SR_ALLOC();
	// 	// OS_CRITICAL_ENTER();
	// 	// pModeIdInfo->isp_key = key;

	// 	ble_system_params.keyInfo.isp_key = key;
	// 	ble_system_params.DirtySystemParamFlg = 1;
	// 	ble_system_info_stored();
	// 	// CPU_CRITICAL_EXIT();
	// }
	// return true;
}
extern void ble_local_restart_net();
bool lc_net_write_key_switch_code(uint8_t key_switch)
{
	u8 key_switch_tmp = ble_system_params.key_switch;
	if(key_switch_tmp != key_switch)
	{
        ble_system_params.key_switch = key_switch;
		ble_system_params.DirtySystemParamFlg = 1;
		ble_system_info_stored();
		ble_local_restart_net();
	}
	return true;
}
void lc_net_get_ble_breaker_addr(uint8_t *buf,uint32_t *len)
{
	*len = sizeof(all_ble_breakers_addr_t);
	memcpy(buf, &all_ble_breakers_addr.breaker_addr[0].mac[0], sizeof(all_ble_breakers_addr_t));	
}
bool lc_net_write_ble_breaker_addr(uint8_t *buf,uint32_t len)
{
	if (len < sizeof(all_ble_breakers_addr_t))
    {
        return false;
    }

	for(int i = 0; i < MAX_BLE_BREAKERS_NUM; i++)
	{
		memcpy(all_ble_breakers_addr.breaker_addr[i].mac, &buf[i * 8], LC_NET_ADDR_SIZE);
		all_ble_breakers_addr.breaker_addr[i].rsv[0] = 0;
		all_ble_breakers_addr.breaker_addr[i].rsv[1] = 0;
	}
	return lc_ble_breaker_addr_stored();
}

//设置出厂配置MAC地址
lc_net_err_t lc_net_set_factory_mac(uint8_t *p_mac,uint8_t len)
{
	if((len>LONG_ADDR_SIZE)||(len <= 0))return LC_NET_EINVAL;
	if((p_mac == NULL))return LC_NET_EINVAL;
	
	uint8_t mac_addr[LONG_ADDR_SIZE];
	memset(mac_addr,0,LONG_ADDR_SIZE);
	memcpy(mac_addr,p_mac,len>LONG_ADDR_SIZE?LONG_ADDR_SIZE:len);
	if((0 != memcmp(mac_addr,pStaChipID->mac,LONG_ADDR_SIZE))
		&&(0 != memcmp(mac_addr,BroadCastMacAddr,LONG_ADDR_SIZE))
	&&(0 != memcmp(mac_addr,InvaildMacAddr,LONG_ADDR_SIZE)))
	{
		memcpy(StaChipID.mac,p_mac,LONG_ADDR_SIZE);
		StorageStaIdInfo();
		return LC_NET_EOK;
	}
	return LC_NET_EINVAL;
}

// bool lc_net_get_neighbor_some_info(uint16_t index, uint32_t *seq, uint16_t *succ_rate)
bool lc_net_get_neighbor_some_info(uint16_t index, uint16_t *succ_rate)
{
	const ST_NEIGHBOR_TAB_TYPE *nghPt = GetNeighborTable();
	if(index >= MAX_NEIGHBOR_NUM)
		return false;

	if(!(nghPt->Neighbor[index].CannotConnect))
	{
		// *seq = nghPt->NeighborList[index].BroadcastSeq;
		*succ_rate = nghPt->Neighbor[index].SucRate[1].UpRate;
		return true;
	}
	return false;
}


//获取当前节点基本信息
void lc_net_get_net_self_info(lc_net_my_info_t *p_info)
{

	const ST_STA_SYS_TYPE *pStaSysPara = GetStaSysPara();
	const ST_STA_ATTR_TYPE* pNetBasePara = GetStaBaseAttr();
	memset(p_info,0,sizeof(lc_net_my_info_t));
        
	if(pNetBasePara->NetState == NET_IN_NET)
	{
		p_info->net_state = LC_NET_IN_STATE;
		memcpy(p_info->addr,pNetBasePara->Mac,LONG_ADDR_SIZE);
		memcpy(p_info->cco_addr,pNetBasePara->CcoMac,LONG_ADDR_SIZE);
		TransposeAddr(p_info->cco_addr);
		p_info->tei = pNetBasePara->TEI;
		p_info->pco_tei = GetPCOTei();
		//
		p_info->role = (lc_net_role_t)(pNetBasePara->Role);
		//
		p_info->nid = pNetBasePara->NID;
		p_info->level = pNetBasePara->Layer;
		p_info->freq_band = GetCurrentHplcNetFreq();
		p_info->reset_time = pStaSysPara->HardRstCnt+pStaSysPara->SofeRstCnt;
	}
	else
	{
		p_info->net_state = LC_NET_OUT_STATE;
		memcpy(p_info->addr,pNetBasePara->Mac,LONG_ADDR_SIZE);
		p_info->freq_band = GetCurrentHplcNetFreq();
		p_info->reset_time = pStaSysPara->HardRstCnt+pStaSysPara->SofeRstCnt;
	}
}

//获取版本配置
void lc_net_get_ver_info(lc_net_ver_info_t *p_ver)
{
	ST_STA_VER_TYPE* pStaVersion = &StaVersion;
    p_ver->boot_ver = pStaVersion->BootVer;
    p_ver->soft_ver = (pStaVersion->SofeVer[1]<<8)|pStaVersion->SofeVer[0];
    p_ver->ver_time_year = pStaVersion->VerDate.Year;
    p_ver->ver_time_mon = pStaVersion->VerDate.Month;
    p_ver->ver_time_day = pStaVersion->VerDate.Day;
    p_ver->factory_code = (pStaVersion->FactoryID[1]<<8)|pStaVersion->FactoryID[0];
    p_ver->chip_code = (pStaVersion->ChipCode[1]<<8)|pStaVersion->ChipCode[0];
	p_ver->net_ver = (u32)StaExtVersion.AppVer;//(MAJ_VER<<24)|(MIN_VER<<16)|(BUILD_VER<<0);
	
	// p_ver->net_ver_year = BUILD_YEAR;
    // p_ver->net_ver_mon= BUILD_MONTH;
    // p_ver->net_ver_day= BUILD_DAY;
    // p_ver->net_ver_hour= BUILD_HOUR;
    // p_ver->net_ver_min= BUILD_MIN;
    // p_ver->net_ver_second= BUILD_SECOND;
}

//获取出厂配置MAC地址
void lc_net_get_factory_mac(uint8_t *p_mac)
{
    memcpy(p_mac,StaChipID.mac,LONG_ADDR_SIZE);
}

//设置自己的网络通信地址
lc_net_err_t lc_net_set_addr(const uint8_t *p_addr,const uint8_t addr_len)
{
	if((addr_len>LONG_ADDR_SIZE)||(addr_len <= 0))return LC_NET_EINVAL;
	if((p_addr == NULL))return LC_NET_EINVAL;
	
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	uint8_t mac_addr[LONG_ADDR_SIZE];
	memset(mac_addr,0,LONG_ADDR_SIZE);
	memcpy(mac_addr,p_addr,addr_len>LONG_ADDR_SIZE?LONG_ADDR_SIZE:addr_len);
	if((0 != memcmp(mac_addr,pNetBasePara->Mac,LONG_ADDR_SIZE))
		&&(0 != memcmp(mac_addr,BroadCastMacAddr,LONG_ADDR_SIZE))
	&&(0 != memcmp(mac_addr,InvaildMacAddr,LONG_ADDR_SIZE)))
	{
		SetStaMAC(mac_addr);
	}
	return LC_NET_EOK;
}

//设置自己的网络通信地址
uint16_t lc_net_get_device_type()
{
	const ST_STA_ATTR_TYPE * pNetBasePara = GetStaBaseAttr();
	return pNetBasePara->DeviceType;
}

//===复位
lc_net_err_t lc_net_reboot_system(const uint32_t delay_ms)
{
	REBOOT_SYSTEM();
	return LC_NET_EOK;
}
//=====================================
//文件升级
//清空文件存储区
void lc_net_clear_file()
{
	LocalStartUpdataFileProcess(0,0,0,0,0);
}
//配置升级文件信息
void lc_net_config_file(lc_net_file_info_t *p_file)
{
	if(p_file->type != LC_NET_LOCAL_FILE)return;
	LocalStartUpdataFileProcess(p_file->id,p_file->size,p_file->crc,p_file->type,p_file->seg_cnt);
}
//写入升级文件段
lc_net_err_t lc_net_write_file_seg(uint32_t seg_offset,uint32_t seg_crc,uint8_t *seg_data,uint32_t seg_len)
{
	return (true == LocalUpdataFileSegmetProcess(seg_offset,seg_crc,seg_data,seg_len))?LC_NET_EOK:LC_NET_ERROR;
}
//
extern void(*lc_file_state_cbfun )(lc_file_state_type);
extern uint32_t (*lc_app_file_check_cbfun)();
void lc_file_set_states_cbfun(void(* state_cbfun)(lc_file_state_type),uint32_t(* check_cbfun)())
{
    lc_file_state_cbfun = state_cbfun;
	lc_app_file_check_cbfun = check_cbfun;
}
//0 返回实际读取字节数
uint32_t lc_net_read_file_addr(void)
{
	extern uint32_t GetUpdateFileAddr(void);
	return GetUpdateFileAddr();
}

//===================================
//网络
void lc_net_start_net()
{
	// if(NET_CLOSED_NET == pNetBasePara.NetState)
	{//若是关闭的，重启组网
		OfflineSetState();
	}
}
//
void lc_net_stop_net()
{
	// if(NET_CLOSED_NET != pNetBasePara.NetState)
	{//非关闭状态，关闭
		OfflineSetState();
		SetNetState(NET_OUT_NET);
	}
}

bool lc_net_is_in_net()
{
	const ST_STA_ATTR_TYPE* pNetBasePara = GetStaBaseAttr();
	return (NET_IN_NET == pNetBasePara->NetState);
}

//===================================

//===================================
//清空存储区    note:flash操作尽量不使用内部调用系统函数的WriteFlash,EraseFlash等函数
lc_net_err_t lc_store_clear(uint32_t start_addr,uint32_t size)
{
	if((size + start_addr) > LC_DATA_APP_PARA_STORE_MAX_SIZE)return LC_NET_EINVAL;
	EraseDataFlash(start_addr, size);
	return LC_NET_EOK;
}
//写入存储区
lc_net_err_t lc_store_write(uint32_t start_addr,uint8_t *p_buff,uint32_t len)
{
	if((len ==0)||((len + start_addr) >LC_DATA_APP_PARA_STORE_MAX_SIZE))return LC_NET_EINVAL;
	WriteDataFlash(start_addr, p_buff,len);
	return LC_NET_EOK;
}
//读取存储区
lc_net_err_t lc_store_read(uint32_t start_addr,uint8_t *p_buff,uint32_t len)
{
	if((len ==0)||((len + start_addr) >LC_DATA_APP_PARA_STORE_MAX_SIZE))return LC_NET_EINVAL;
	ReadDataFlash(start_addr, p_buff, len);
	return LC_NET_EOK;
}
//key存储
bool ble_key_info_stored()
{
	flash_store_head_st store_head;
	DataFlashInit();
    // EraseDataFlash(LC_IP_STORE_ID_INFO_FLASH_START_ADDR, sizeof(ST_MODE_ID_INFO_TYPE) + DATA_FLASH_PAGE_SIZE);
    EraseDataFlash(LC_IP_STORE_ID_INFO_FLASH_START_ADDR, DATA_FLASH_PAGE_SIZE + DATA_FLASH_PAGE_SIZE);
    
    store_head.Head = BLE_STORE_HEAD_FLG;
    store_head.DataIdx = 0;
    store_head.DataLen = sizeof(ST_MODE_ID_INFO_TYPE);
    store_head.Crc32 = crc32_cal((uint8_t*)&IdInfo, store_head.DataLen);

    WriteDataFlash(LC_IP_STORE_ID_INFO_FLASH_START_ADDR + DATA_FLASH_PAGE_SIZE, (uint8_t*)&IdInfo,sizeof(ST_MODE_ID_INFO_TYPE));
    WriteDataFlash(LC_IP_STORE_ID_INFO_FLASH_START_ADDR, (uint8_t*)&store_head,sizeof(flash_store_head_st));
	DataFlashClose();
    return true;
}

bool ble_read_key_info(void)
{
    flash_store_head_st store_head;
    uint32_t crc32;
    bool rflg = true;
    ReadDataFlash(LC_IP_STORE_ID_INFO_FLASH_START_ADDR, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    if((BLE_STORE_HEAD_FLG != store_head.Head)||(store_head.DataLen != sizeof(ST_MODE_ID_INFO_TYPE)))
    {
        rflg = false;
    }
    else
    {
        ReadDataFlash(LC_IP_STORE_ID_INFO_FLASH_START_ADDR+ DATA_FLASH_PAGE_SIZE, (uint8_t*)&IdInfo,store_head.DataLen);

        crc32 = crc32_cal((uint8_t*)&IdInfo, store_head.DataLen);
    }
    if(crc32 != store_head.Crc32)
    {
        rflg = false;
    }
    return rflg;
}

//初始化key参数
void ble_init_key_info_from_store()
{
    if(false == ble_read_key_info())
    {
		IdInfo.isp_key = 0;
		IdInfo.proj_key = 0;
		IdInfo.ex_param = 1;
        ble_key_info_stored();
    }
}


bool lc_ble_breaker_addr_stored()
{
	flash_store_head_st store_head;
	DataFlashInit();
    EraseDataFlash(LC_BLE_BREAKER_MAC_START_ADDR, DATA_FLASH_PAGE_SIZE + DATA_FLASH_PAGE_SIZE);
    
    store_head.Head = BLE_STORE_HEAD_FLG;
    store_head.DataIdx = 0;
    store_head.DataLen = sizeof(all_ble_breakers_addr_t);
    store_head.Crc32 = crc32_cal((uint8_t*)&all_ble_breakers_addr, store_head.DataLen);

    WriteDataFlash(LC_BLE_BREAKER_MAC_START_ADDR + DATA_FLASH_PAGE_SIZE, (uint8_t*)&all_ble_breakers_addr,sizeof(all_ble_breakers_addr_t));
    WriteDataFlash(LC_BLE_BREAKER_MAC_START_ADDR, (uint8_t*)&store_head,sizeof(flash_store_head_st));
	DataFlashClose();
    return true;
}

bool lc_read_ble_breaker_addr(void)
{
    flash_store_head_st store_head;
    uint32_t crc32;
    bool rflg = true;
    ReadDataFlash(LC_BLE_BREAKER_MAC_START_ADDR, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    if((BLE_STORE_HEAD_FLG != store_head.Head)||(store_head.DataLen != sizeof(all_ble_breakers_addr_t)))
    {
        rflg = false;
    }
    else
    {
        ReadDataFlash(LC_BLE_BREAKER_MAC_START_ADDR + DATA_FLASH_PAGE_SIZE, (uint8_t*)&all_ble_breakers_addr,store_head.DataLen);

        crc32 = crc32_cal((uint8_t*)&all_ble_breakers_addr, store_head.DataLen);
    }
    if(crc32 != store_head.Crc32)
    {
        rflg = false;
    }
    return rflg;
}

//初始化key参数
void lc_init_ble_breaker_addr_from_store()
{
    if(false == lc_read_ble_breaker_addr())
    {
		memset(&all_ble_breakers_addr, 0, sizeof(all_ble_breakers_addr_t));
        lc_ble_breaker_addr_stored();
    }
	AddBleBreakerToSystemMeterList();
}
//====================================
//内存管理
void ble_molloc_init()
{
    //内存池初始化
    OS_ERR err;
    OSMemCreate(&Blepool,"Block BleLocal Memory",Blepdu, MAX_RECEIVE_DATA_NUM, sizeof(Ble_Mem_Union), &err);
    if(err != OS_ERR_NONE)
    {
        while(1);
    }
}

//分配一个ble black
Ble_Mem_Union* BleMalloc(uint32_t size)
{
	if (size > MAX_BLE_DATA_SIZE)
	{
		//$$$ err
		return NULL;
	}
	
    OS_ERR err;
    Ble_Mem_Union *mem_ptr = 0;
    mem_ptr = (Ble_Mem_Union*)OSMemGet(&Blepool,&err);
    if(err != OS_ERR_NONE){
#ifdef __DEBUG_MODE
			while(1)
			{
				__disable_irq();
				FeedWdg();
			}
#else
			return (void *)0;
#endif	
	}
    return mem_ptr;
}
//释放一个ble black
void BleFree(Ble_Mem_Union* pdu)
{
    OS_ERR err;
    OSMemPut(&Blepool,pdu,&err);
}