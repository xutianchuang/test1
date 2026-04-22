#include "ble_data.h"
#include "lc_all.h"
#include "protocol_includes.h"
#include "os.h"
#include "common_includes.h"
#include "crclib.h"
#include "bps_flash.h"
#include "ble_update.h"


ble_system_params_type ble_system_params;
ble_neighbor_info_st_type ble_neighbor_list[BLE_MAX_NEIGHBOR_NUM];
ble_net_info_st_type ble_net_info;
ble_sta_net_info_st_type ble_sta_net_info;
uint16_t DataTeiList[BLE_MAX_NEIGHBOR_NUM];//用于节点列表，临时处理
//系统信息及档案参数
static TimeValue  ble_local_syn_timer;                //存储同步定时器
#define BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME	(5000UL)


//提供物模型获取接口
uint32_t ble_get_report_wait_time()
{
    return ble_system_params.lamp_report_waite_time;
}

uint32_t ble_get_report_event_en_mask()
{
    return ble_system_params.event_en_mask;
}

bool ble_put_report_wait_time(uint32_t waite_time)
{
    ble_system_params.lamp_report_waite_time = waite_time;
    return TRUE;
}

bool ble_put_report_event_en_mask(uint32_t en_mask)
{

    ble_system_params.event_en_mask = en_mask;
    return TRUE;
}

bool ble_change_ble_send_power(uint32_t ble_power)
{
    if(ble_power > 10) return FALSE;
    ble_system_params.ble_power = ble_power;
    //保存flash

    // 配置蓝牙功率
    lamp_ble_power_set(ble_system_params.ble_power);
    return TRUE;
}

uint16_t ble_get_ble_send_power()
{
    return ble_system_params.ble_power;
}
//清除非下载的地址
static void ble_clear_not_down_mac_addr()
{
    int macsum = ble_system_params.MacAddrSum;
    for(int i = macsum-1;i>=0;i--)
    {
        if((ble_system_params.MacAddrList[i].MacAttr != BLE_MAC_ADDR_NORMAL_ATTR)
        &&(ble_system_params.MacAddrList[i].MacAttr != BLE_MAC_ADDR_RELAY_ATTR))
        {
            ble_delete_mac_addr_from_list(ble_system_params.MacAddrList[i].MacAddr,true);
        }
    }
    ble_system_params.SystemParamsStatus = PARAM_MAC_CHANGE_PARAM_STATUS;
}

static void ble_check_not_down_mac_addr()
{
    int macsum = ble_system_params.MacAddrSum;
    uint32_t nornodesum = 0;
    uint32_t relnodesum = 0;

    for(int i = macsum-1;i>=0;i--)
    {
        if(ble_system_params.MacAddrList[i].MacAttr == BLE_MAC_ADDR_NORMAL_ATTR)
        {
            nornodesum++;
        }
        else if(ble_system_params.MacAddrList[i].MacAttr == BLE_MAC_ADDR_RELAY_ATTR)
        {
            relnodesum++;
        }
        else
        {
            ble_delete_mac_addr_from_list(ble_system_params.MacAddrList[i].MacAddr,true);
        }
    }
    if((nornodesum != ble_system_params.MacAddrNormalSum)
    ||(relnodesum != ble_system_params.MacAddrRelaySum)
    ||((nornodesum + relnodesum) != ble_system_params.MacAddrSum))
    {
        memset(&(ble_system_params.MacAddrList[0]),0x00,sizeof(ble_system_params.MacAddrList));
        ble_system_params.MacAddrSum = 0;
        ble_system_params.MacAddrNormalSum = 0;
        ble_system_params.MacAddrFindedSum = 0;
        ble_system_params.MacAddrNeedDelSum = 0;
        ble_system_params.MacAddrRelaySum = 0;
    }
}

static void ble_clear_not_attr_mac_addr(uint8_t attr)
{
    int macsum = ble_system_params.MacAddrSum;
    //int leavemaccnt = 0;
    for(int i = macsum-1;i>=0;i--)
    {
        if(ble_system_params.MacAddrList[i].MacAttr != attr)
        {
            ble_delete_mac_addr_from_list(ble_system_params.MacAddrList[i].MacAddr,true);
        }
    }
    ble_system_params.SystemParamsStatus = PARAM_MAC_CHANGE_PARAM_STATUS;
}

//清除Mac地址档案表
void ble_clear_mac_addr_list()
{
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();

    //StopHplcNetRun();
   //InitHplcNetParam();
#if defined(HPLC_NET_ALL_WHITE_ADDR_MODEL_ENABLE_FLG)
    ble_system_params.MacAddrSum = 0;
    ble_system_params.MacAddrNormalSum = 0;
    ble_system_params.MacAddrFindedSum = 0;
    ble_system_params.MacAddrNeedDelSum = 0;
    ble_system_params.MacAddrRelaySum = 0;
#else
    if(ble_system_params.MacAddrRelaySum == 0)
    {
        ble_system_params.MacAddrSum = 0;
        ble_system_params.MacAddrNormalSum = 0;
        ble_system_params.MacAddrFindedSum = 0;
        ble_system_params.MacAddrNeedDelSum = 0;
        ble_system_params.MacAddrRelaySum = 0;
    }
    else
    {
        ble_clear_not_attr_mac_addr(BLE_MAC_ADDR_RELAY_ATTR);
    }
#endif
    //
    ble_system_params.DirtySystemParamFlg = 1;
    ble_system_params.SystemParamsStatus = PARAM_MAC_CLEARED_LIST_STATUS;

    OS_CRITICAL_EXIT();
	//ble_store_system_info();
    StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
}


//加入一个地址；（已加入，马上加入，空间已满）
//删除一个地址；(已删除，未在档案中)
//查找地址;(在地址列表中，不在地址列表中，返回索引值),>=0查找成功，-1查找失败；
int ble_sreach_mac_addr_list(const uint8_t *macaddr,const ble_mac_addr_type *macaddrlist,const uint16_t macsum,int *inserti)
{
    int left=0;
    int right;
    int mid = 0;
    left = 0;
    right = macsum - 1;
    while (left <= right)
    {
        mid = left + ((right - left) >> 1);
        int cmpr = memcmp(macaddrlist[mid].MacAddr,macaddr,BLE_LONG_ADDR_SIZE);
        if (cmpr > 0)
        {
            right = mid - 1;
        }
        else if (cmpr < 0)
        {
            left = mid + 1;
        }
        else
            return mid;//找到了，返回下标
    }
    if(inserti != NULL)
    {
        *inserti = mid;
    }
    return -1;//找不到
}

////校验midx与tei的关联准确性
//bool CheckMacAddrListAndNeightList(uint16_t midx,uint16_t tei)
//{
//    uint16_t macidx = midx;
//    uint16_t chtei = tei;
//    bool rflg = true;
//    if(macidx < BLE_MAX_NODE_MAC_ADDR_NUM)
//    {
//        if((chtei >= BLE_MIN_TEI_VALUE)
//        &&(chtei <= BLE_MAX_TEI_VALUE))
//        {
//            //midx校验tei    
//            if(ble_system_params.MacAddrList[macidx].NeightIndex != chtei)rflg = false;
//        }
//        else
//        {
//            uint16_t ctei = ble_system_params.MacAddrList[macidx].NeightIndex;        
//        }
//    }
//    
//    if((rflg == true)&&(chtei >= BLE_MIN_TEI_VALUE)
//    &&(chtei <= BLE_MAX_TEI_VALUE))
//    {
//        //tei校验midx
//        ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
//        uint16_t macaddrsum = ble_system_params.MacAddrSum;
//        int macidx = 0;
//        int insertindex = 0;

//        uint8_t macaddr[BLE_LONG_ADDR_SIZE];
//        GetNeighborInfoExtMember(chtei,macaddr,GetMemberOff(ST_NEIGHBOR_INFO_EXTRA,Mac),BLE_LONG_ADDR_SIZE);

//        if((macidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,&insertindex)) == -1)
//        {//不在档案内
//            rflg = false;
//        }
//        else
//        {
//            //midx校验tei    
//            if(ble_system_params.MacAddrList[macidx].NeightIndex != chtei)rflg = false;
//        }
//        if(ble_system_params.MacAddrList[macidx].NeightIndex != chtei)rflg = false;
//    }

//    if(rflg == false)
//    {
//        #if defined(DEBUG)
//		DEBUG_PROCESS();
//        #else
//        REBOOT_SOFT_EXP(REBOOT_CCO_TEI_TO_MAC_ERR,NULL);
//        #endif
//    }
//    return rflg;
//    
//}

//插入表地址到档案列表中
int ble_insert_mac_addr_to_list(const uint8_t *macaddr,const uint8_t prot,uint8_t attr)
{
    ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    //uint8_t *pmacaddr = (uint8_t*)macaddrlist;

    if(ble_system_params.MacAddrSum >= BLE_MAX_NODE_MAC_ADDR_NUM)
    {
        if(((ble_system_params.MacAddrNormalSum + ble_system_params.MacAddrRelaySum) < ble_system_params.MacAddrSum)
        &&((attr == BLE_MAC_ADDR_NORMAL_ATTR)||(BLE_MAC_ADDR_RELAY_ATTR == attr)))
        {
            ble_clear_not_down_mac_addr();
        }
        else
        {
            return -1;
        }
    }
       CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int macidx = 0;
    int insertindex = 0;
    if((macidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,&insertindex)) == -1)
    {//不在档案内，可以插入
        if ((macaddrsum >0)&&(memcmp(macaddrlist[insertindex].MacAddr,macaddr,BLE_LONG_ADDR_SIZE) < 0))
        {
            ++insertindex;
        }
        if(macaddrsum >= 1)
        {
            int movesum = (macaddrsum - insertindex);
            int moveidex = macaddrsum - 1;
            for(int i=0;i<movesum;i++)
            {
                memcpy(&macaddrlist[moveidex+1-i],&macaddrlist[moveidex-i],sizeof(ble_mac_addr_type));
            }
        }
        
        macaddrsum++;

        memcpy(macaddrlist[insertindex].MacAddr,macaddr,BLE_LONG_ADDR_SIZE);
        memset(macaddrlist[insertindex].LocalMacAddr,0xFF,BLE_LONG_ADDR_SIZE);
        macaddrlist[insertindex].MacProt = prot;
        macaddrlist[insertindex].MacAttr = attr;
        macaddrlist[insertindex].NeightIndex = BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE;
        ble_system_params.MacAddrSum = macaddrsum;

        switch(attr)
        {
            case BLE_MAC_ADDR_NORMAL_ATTR:
                ble_system_params.MacAddrNormalSum++;
            break;
            case BLE_MAC_ADDR_FINDED_ATTR:
                ble_system_params.MacAddrFindedSum++;
            break;
            case BLE_MAC_ADDR_NEED_DELETE_ATTR:
                ble_system_params.MacAddrNeedDelSum++;
            break;
            case BLE_MAC_ADDR_RELAY_ATTR:
                ble_system_params.MacAddrRelaySum++;
            break;
            default:
            break;
        }
        
        ble_system_params.DirtySystemParamFlg = 1;
        StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
    }
    else
    {//已在档案内
        insertindex = macidx;
        //macidx
        switch(ble_system_params.MacAddrList[macidx].MacAttr)
        {
            case BLE_MAC_ADDR_NORMAL_ATTR:
                ble_system_params.MacAddrNormalSum--;
            break;
            case BLE_MAC_ADDR_FINDED_ATTR:
                ble_system_params.MacAddrFindedSum--;
            break;
            case BLE_MAC_ADDR_NEED_DELETE_ATTR:
                ble_system_params.MacAddrNeedDelSum--;
            break;
            case BLE_MAC_ADDR_RELAY_ATTR:
                ble_system_params.MacAddrRelaySum--;
            break;
            default:
            break;
        }
        
        switch(attr)
        {
            case BLE_MAC_ADDR_NORMAL_ATTR:
                ble_system_params.MacAddrNormalSum++;
            break;
            case BLE_MAC_ADDR_FINDED_ATTR:
                ble_system_params.MacAddrFindedSum++;
            break;
            case BLE_MAC_ADDR_NEED_DELETE_ATTR:
                ble_system_params.MacAddrNeedDelSum++;
            break;
            case BLE_MAC_ADDR_RELAY_ATTR:
                ble_system_params.MacAddrRelaySum++;
            break;
            default:
            break;
        }
        
        if((macaddrlist[macidx].MacAttr != attr)||
        (macaddrlist[macidx].MacProt != prot))
        {
            macaddrlist[macidx].MacAttr = attr;
            macaddrlist[macidx].MacProt = prot;
            if((BLE_MAC_ADDR_NORMAL_ATTR == attr)
            ||(BLE_MAC_ADDR_RELAY_ATTR == attr))
            {
                ble_system_params.DirtySystemParamFlg = 1;
                StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
            }
        }
    }
    OS_CRITICAL_EXIT();
    return insertindex;
}
//-1,不在档案内，其它在档案内
int ble_check_mac_addr_from_list(const uint8_t *macaddr)
{
	CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
	OS_CRITICAL_EXIT();
	return mlidx;
}


//从地址列表中删除表地址,flg=true强制删除，false正常删除
int ble_delete_mac_addr_from_list(const uint8_t *macaddr,uint8_t flg)
{
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
    if(-1 != mlidx)
    {//在档案内，可以删除
        ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[mlidx];
        switch(macaddrinfo->MacAttr)
        {
            case BLE_MAC_ADDR_NORMAL_ATTR:
                ble_system_params.MacAddrNormalSum--;
            break;
            case BLE_MAC_ADDR_FINDED_ATTR:
                ble_system_params.MacAddrFindedSum--;
            break;
            case BLE_MAC_ADDR_NEED_DELETE_ATTR:
                ble_system_params.MacAddrNeedDelSum--;
            break;
            case BLE_MAC_ADDR_RELAY_ATTR:
                ble_system_params.MacAddrRelaySum--;
            break;
            default:
            break;
        }
        
//        if((macaddrinfo->NeightIndex == BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
//            ||(flg == true))
        if(flg == true)
        {
            int movesum = (macaddrsum - mlidx - 1);
            int moveidex = mlidx;
            for(int i=0;i<movesum;i++)
            {
                memcpy(&macaddrlist[moveidex+i],&macaddrlist[moveidex+1+i],sizeof(ble_mac_addr_type));
            }
            macaddrsum--;
            ble_system_params.MacAddrSum = macaddrsum;
            ble_system_params.DirtySystemParamFlg = 1;
            StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
        }
        else
        {//已分配TEI
            if(macaddrinfo->MacAttr != BLE_MAC_ADDR_NEED_DELETE_ATTR)
            {
                ble_system_params.DirtySystemParamFlg = 1;
                StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
            }
            macaddrinfo->MacAttr = BLE_MAC_ADDR_NEED_DELETE_ATTR;
            ble_system_params.MacAddrNeedDelSum++;
        }
    }
    else
    {//已不在档案内
        
    }
    OS_CRITICAL_EXIT();
    return mlidx;
}

//插入表地址到档案列表中
int ble_insert_local_mac_addr_to_list(const uint8_t *macaddr,const uint8_t *localmacaddr)
{
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
    if(-1 != mlidx)
    {//在档案内，可以删除
        ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[mlidx];
        if(0 != memcmp(macaddrinfo->LocalMacAddr,localmacaddr,BLE_LONG_ADDR_SIZE))
        {
            memcpy(macaddrinfo->LocalMacAddr,localmacaddr,BLE_LONG_ADDR_SIZE);
            ble_system_params.DirtySystemParamFlg = 1;
            //StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
        }
    }
    OS_CRITICAL_EXIT();
    return mlidx;
}
//获取本地MAC地址
int ble_get_local_mac_addr_from_list(const uint16_t macaddridx,uint8_t *macaddr,uint8_t *localmacaddr)
{
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    //ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int macaddrreadidx = macaddridx;
    if(macaddrreadidx >= macaddrsum)
    {
        macaddrreadidx = -1;
    }
    else
    {
        ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[macaddrreadidx];
        memcpy(macaddr,macaddrinfo->MacAddr,BLE_LONG_ADDR_SIZE);
        memcpy(localmacaddr,macaddrinfo->LocalMacAddr,BLE_LONG_ADDR_SIZE);
    }
    OS_CRITICAL_EXIT();
    return macaddrreadidx;
}
//==
//存储网络参数到Flash
static void ble_store_system_info(void)
{
    flash_store_head_st store_head;
	DataFlashInit();
    // lc_store_clear(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR, sizeof(ble_system_params_type) + DATA_FLASH_PAGE_SIZE);
    EraseDataFlash(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR, sizeof(ble_system_params_type) + DATA_FLASH_PAGE_SIZE);
    
    store_head.Head = BLE_STORE_HEAD_FLG;
    store_head.DataIdx = 0;
    store_head.DataLen = sizeof(ble_system_params_type);
    store_head.Crc32 = crc32_cal((uint8_t*)&ble_system_params, store_head.DataLen);
    
    // uint8_t *wdatapoint = (uint8_t*)&ble_system_params;
    // uint32_t datalen = sizeof(ble_system_params_type);
    //uint32_t currentwritedatalen = DATA_FLASH_SECTION_SIZE - DATA_FLASH_PAGE_SIZE;
    //uint32_t currentwritedataindex = 0;
    //uint32_t sectioncnt = 1;

    // lc_store_write(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR + DATA_FLASH_PAGE_SIZE, wdatapoint, datalen);
    // lc_store_write(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    WriteDataFlash(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR + DATA_FLASH_PAGE_SIZE, (uint8_t*)&ble_system_params,sizeof(ble_system_params_type));
    WriteDataFlash(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR, (uint8_t*)&store_head,sizeof(flash_store_head_st));
	DataFlashClose();

}

//从Flash读取系统参数
static bool ble_read_system_info(void)
{
    flash_store_head_st store_head;
    uint32_t crc32;
    bool rflg = true;
    // OS_CRITICAL_ENTER();
    // ReadDataFlash(DATA_STA_SYSYEM_PARAM_STORE_ADDR, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    // lc_store_read(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    ReadDataFlash(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR, (uint8_t*)&store_head, sizeof(flash_store_head_st));
    if((BLE_STORE_HEAD_FLG != store_head.Head)||(store_head.DataLen != sizeof(ble_system_params_type)))
    {
        rflg = false;
    }
    else
    {
        // ReadDataFlash(DATA_STA_SYSYEM_PARAM_STORE_ADDR + DATA_FLASH_PAGE_SIZE, (uint8_t*)&ble_system_params, store_head.DataLen);
        // lc_store_read(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR + DATA_FLASH_PAGE_SIZE, (uint8_t*)&ble_system_params, store_head.DataLen);
        ReadDataFlash(LC_IP_STORE_SYSTEM_PARAM_DATA_FLASH_START_ADDR+ DATA_FLASH_PAGE_SIZE, (uint8_t*)&ble_system_params,store_head.DataLen);

        crc32 = crc32_cal((uint8_t*)&ble_system_params, store_head.DataLen);
    }
    // OS_CRITICAL_EXIT();
    if(crc32 != store_head.Crc32)
    {
        rflg = false;
    }
    return rflg;
}

//初始化数据为默认数据
static void ble_init_default_system_info(uint8_t *long_addr)
{
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();

    memset(&ble_system_params,0x00,sizeof(ble_system_params));
    uint32_t randaddr = rand();

    ble_system_params.lamp_report_waite_time = 60; //60s
    ble_system_params.event_en_mask = 0xFFFFFFFF;
    ble_system_params.ble_power = 6;    //默认为6

//	ble_system_params.SystemLongAdd[0] = (randaddr >> 0 )&0xFF;
//  ble_system_params.SystemLongAdd[1] = (randaddr >> 8 )&0xFF;
//  ble_system_params.SystemLongAdd[2] = (randaddr >> 16 )&0xFF;
//  ble_system_params.SystemLongAdd[3] = (randaddr >> 24 )&0xFF;
//  ble_system_params.SystemLongAdd[4] = 0xCC;
//  ble_system_params.SystemLongAdd[5] = 0xCC;
	
	ble_system_params.NetEnableFlag = 1;
	uint32_t param = lc_net_get_ex_param_code();
	if(ble_system_params.NetEnableFlag == 0)
	{
		param &= ~(0x01UL);
	}
	else
	{
		param |= 0x01;
	}
	lc_net_write_ex_param_code(param);
	
	memcpy(ble_system_params.SystemLongAdd,long_addr,LC_NET_ADDR_SIZE);
	ble_system_params.NetId = crc16_cal(ble_system_params.SystemLongAdd,BLE_LONG_ADDR_SIZE);
	if(ble_system_params.NetId == 0)
	{
		ble_system_params.NetId = rand();
	}
	
    //
//    ble_system_params.FactoryCode = ((uint16_t)FixedAssetsInfo.ModuleFactoryID[1]<<8)|((uint16_t)FixedAssetsInfo.ModuleFactoryID[0]<<0);
//    ble_system_params.ChipCode = ((uint16_t)FixedAssetsInfo.ChipFactoryID[1]<<8)|((uint16_t)FixedAssetsInfo.ChipFactoryID[0]<<0);
//    ble_system_params.VersionValue = (((uint32_t)FixedAssetsInfo.ChipHardwareReleaseDate[2])<<16)|(((uint32_t)FixedAssetsInfo.ChipHardwareReleaseDate[1])<<8)|(((uint32_t)FixedAssetsInfo.ChipHardwareReleaseDate[0])<<0);;
//    ble_system_params.SoftVersion = ((uint32_t)FixedAssetsInfo.ModuleSofeVer[1]<<8)|((uint32_t)FixedAssetsInfo.ModuleSofeVer[0]<<0);;
    //
    ble_system_params.MacAddrSum = 0;
    ble_system_params.MacAddrNormalSum = 0;
    ble_system_params.MacAddrRelaySum = 0;
    ble_system_params.MacAddrFindedSum = 0;
    ble_system_params.MacAddrNeedDelSum = 0;
    
    ble_system_params.SystemParamsStatus = PARAM_MAC_CHANGE_PARAM_STATUS;

	ble_system_params.ble_sta_mode = 0;
	memset(&ble_system_params.meters_addr[0].mac[0],0,20*8);
	ble_system_params.key_switch = 0;
    OS_CRITICAL_EXIT();
}
//初始化系统参数
static void ble_init_system_info_from_store()
{
	lc_net_my_info_t p_info;
	memset(&p_info,0,sizeof(lc_net_my_info_t));
	lc_net_get_net_self_info(&p_info);
	
    //dsp_memcpy(pdst,p_info.addr,LC_NET_ADDR_SIZE);
	
    if(false == ble_read_system_info())
    {
        ble_init_default_system_info(p_info.addr);
    }
    else
    {
		if(0 != memcmp(ble_system_params.SystemLongAdd,p_info.addr,LC_NET_ADDR_SIZE))
		{
			ble_init_default_system_info(p_info.addr);
			//memcpy(ble_system_params.SystemLongAdd,p_info.addr,LC_NET_ADDR_SIZE);
		}
		ble_check_not_down_mac_addr();
    }
    //
	ble_system_params.NetEnableFlag = lc_net_get_ex_param_code()&0x01;
	//
    ble_system_params.DirtySystemParamFlg = 1;
    ble_system_params.SystemParamsStatus = PARAM_MAC_POWERUP_LIST_STATUS;
    //
    if(ble_system_params.lamp_report_waite_time >= 60000)
    {ble_system_params.lamp_report_waite_time = 60;}
    StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
}


//上电初始化系统参数
void ble_system_info_reset_init()
{
    ble_init_key_info_from_store();
    ble_init_system_info_from_store();

	lc_init_ble_breaker_addr_from_store();
    //
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    uint32_t readnseed = ble_system_params.SystemLongAdd[0] + ble_system_params.SystemLongAdd[1];
    readnseed = (readnseed<<9)|(ble_system_params.SystemLongAdd[2] + ble_system_params.SystemLongAdd[3]);
    readnseed = (readnseed<<9)|(ble_system_params.SystemLongAdd[4] + ble_system_params.SystemLongAdd[5]);
    srand(readnseed);
    //ble_system_params.NetSeq++;
	ble_system_params.HardRstCnt++;
    OS_CRITICAL_EXIT();
}
//===
bool ble_system_info_stored()
{
	if(ble_system_params.DirtySystemParamFlg != 0)
	{
		CPU_SR_ALLOC();
        OS_CRITICAL_ENTER();
		ble_system_params.DirtySystemParamFlg = 0;
		OS_CRITICAL_EXIT();
		ble_store_system_info();
		return true;
	}
	return false;
}
//
bool ble_set_system_net_id(uint32_t net_id)
{
	ble_system_params.DirtySystemParamFlg = 1;
	ble_system_params.NetId = net_id;
	StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
	return true;
}

//存储参数任务处理
bool ble_system_info_syn_process()
{
    bool rflg = false;
    if(LcTimeOut(&ble_local_syn_timer))
    {//处理系统参数,存储及同步档案
		rflg = ble_system_info_stored();
		StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
    }
    return rflg;
}
//
bool ble_system_info_solt_time_store()
{
	ble_system_params.DirtySystemParamFlg = 1;
	StartTimer(&ble_local_syn_timer,BRUSH_LOCAL_SYSTEM_PARAM_SYN_TIME);
	return true;
}
//========================================================================
//根据mac地址分配一个tei地址,0无效
uint16_t ble_malloc_tei_from_mac_addr(uint8_t *macaddr)
{
    uint16_t tei = 0;
    ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
    if(-1 != mlidx)
    {//在档案内
        ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[mlidx];
        if(macaddrinfo->MacAttr != BLE_MAC_ADDR_NEED_DELETE_ATTR)
        {
            if(macaddrinfo->NeightIndex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
            {
                tei = macaddrinfo->NeightIndex;
            }
            else
            {
                for(int i=(BLE_CCO_TEI+1);i<BLE_MAX_NEIGHBOR_NUM;i++)
                {
                    if(ble_neighbor_list[i].NeightState != NEIGHT_HAVE_FLG_VALUE)
                    {
                        tei = i;
                        //
                        CPU_SR_ALLOC();
                        OS_CRITICAL_ENTER();
                        macaddrinfo->NeightIndex = i;
                        memset(&ble_neighbor_list[i],0x00,sizeof(ble_neighbor_info_st_type));
                        ble_neighbor_list[i].NeightState = NEIGHT_HAVE_FLG_VALUE;
						memcpy(ble_neighbor_list[i].MacAddr,macaddrinfo->MacAddr,BLE_LONG_ADDR_SIZE);
                        OS_CRITICAL_EXIT();
                        break;
                    }
                }
           }
        }
    }
    return tei;
}
//根据macaddr或tei释放tei;
bool ble_free_tei_and_mac_addr(uint8_t *macaddr,uint16_t tei)
{
    bool rflg = false;
    if(macaddr != NULL)
    {
        ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
        uint16_t macaddrsum = ble_system_params.MacAddrSum;
        int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
        if(-1 != mlidx)
        {//档案内
            ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[mlidx];
            uint16_t teiidx = macaddrinfo->NeightIndex;
            if(teiidx != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
            {//有tei
                ble_neighbor_list[teiidx].NeightState = NEIGHT_NOTHAVE_FLG_VALUE;//释放
                macaddrinfo->NeightIndex = BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE;
            }
            rflg = true;
        }
    }
    else
    {
        if((tei> BLE_MIN_TEI_VALUE)&&(tei <= BLE_MAX_TEI_VALUE))
        {
            ble_neighbor_info_st_type *pnbinfo = &ble_neighbor_list[tei];
            ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
            uint16_t macaddrsum = ble_system_params.MacAddrSum;
            //
            int mlidx = ble_sreach_mac_addr_list(pnbinfo->MacAddr,macaddrlist,macaddrsum,NULL);
            if(-1 != mlidx)
            {//档案内
                ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[mlidx];
                pnbinfo->NeightState = NEIGHT_NOTHAVE_FLG_VALUE;//释放
                macaddrinfo->NeightIndex = BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE;
                //==
                rflg = true;
            }
        }
    }
    return rflg;
}

//======================================
//配置节点从离线到入网
bool ble_set_tei_in_net_stay_state(uint16_t tei)
{
    if((tei < BLE_MIN_TEI_VALUE) || (tei > BLE_MAX_TEI_VALUE))
        return false;
    if(ble_neighbor_list[tei].NeightState != NEIGHT_HAVE_FLG_VALUE)
        return false;//未分配的，异常
    if(ble_neighbor_list[tei].NetState != NET_OFFLINE_NET)
        return false;

    ble_neighbor_list[tei].NetState = NET_IN_NET;//入网
    ++ble_net_info.HpclStayNetSum;
    //
    OS_ERR err;
    ble_net_info.HPlcBuildNetLastStayNetTicks = OSTimeGet(&err);
    //{tick\tC\tBIN\tTei=tei}\n ：//tei在线
    //CCO_PRINTF("{%d\tC\tBIN\tTei=%d}\r\n",rt_tick_get(),tei);    
    //
#ifdef STAY_BIT_MAP_TST
    if((HplcNetStayTeiBitMap[tei>>3] & 0x01<<(tei&0x07)) ==0x00)
    {
        HplcNetStayTeiBitMap[tei>>3] |= 0x01<<(tei&0x07);//置位
    }
    else
    {//已置位
        rt_kprintf("%d\t!!!TeiInNet tei:%d\n",rt_tick_get(),tei);
        while(1);
    }
#endif
    uint16_t m485tei = ble_neighbor_list[tei].Meter485ListNextTei;
    while(m485tei != 0)
    {
        ble_set_tei_in_net_stay_state(m485tei);
        m485tei = ble_neighbor_list[m485tei].Meter485ListNextTei;
    }
    return true;
}
//配置tei节点离线状态
bool ble_set_tei_out_net_stay_state(uint16_t tei)
{
    if((tei < BLE_MIN_TEI_VALUE) || (tei > BLE_MAX_TEI_VALUE))return  false;
    if(ble_neighbor_list[tei].NeightState != NEIGHT_HAVE_FLG_VALUE)return false;//未分配的，异常
    if(ble_neighbor_list[tei].NetState != NET_IN_NET)return false;
    ble_neighbor_list[tei].NetState = NET_OFFLINE_NET;//离线
    --ble_net_info.HpclStayNetSum;
    //{tick\tC\tBON\tTei=tei}\n ：//tei离线
    //CCO_PRINTF("{%d\tC\tBON\tTei=%d}\r\n",rt_tick_get(),tei);
#ifdef STAY_BIT_MAP_TST
    if((HplcNetStayTeiBitMap[tei>>3] & 0x01<<(tei&0x07)) !=0x00 )
    {
        HplcNetStayTeiBitMap[tei>>3] &= ~(0x01<<(tei&0x07));//
    }
    else
    {//已置位
        rt_kprintf("%d\t!!!LTeiOutNet tei:%d\r\n",rt_tick_get(),tei);
        while(1);
    }
#endif
    uint16_t m485tei = ble_neighbor_list[tei].Meter485ListNextTei;
    while(m485tei != 0)
    {
        ble_set_tei_out_net_stay_state(m485tei);
        m485tei = ble_neighbor_list[m485tei].Meter485ListNextTei;
    }
    return true;
}

//==============================
//主要路由表操作
//功能：查找tei是否在pcotei直接子站点，若在返回ture,prior返回上一跳兄弟节点
//false不在pcotei下
bool ble_find_tei_is_pco_child(uint16_t tei,uint16_t pcotei,uint16_t *prior)
{
    uint16_t pointtei = ble_neighbor_list[pcotei].DirectChilTei;
    uint16_t priortei = 0;
    while(pointtei != 0)
    {
        if(pointtei == tei)
        {
            if(prior != NULL)
            {
                *prior = priortei;
            }
            return true;
        }
        priortei = pointtei;
        pointtei = ble_neighbor_list[pointtei].BrotherNextTei;
    }
    return false;
}
//把tei插入pcotei直接子站点下
bool ble_insert_tei_to_pco(uint16_t tei,uint16_t pcotei)
{
    //CCO_PRINTF("{%d\tC\tJP\tTei=%d\tPco=%d\tOPco=%d}\r\n",rt_tick_get(),tei,pcotei,ble_neighbor_list[tei].NeighborPcoTEI);
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    ble_neighbor_list[tei].BrotherNextTei = ble_neighbor_list[pcotei].DirectChilTei;
    ble_neighbor_list[pcotei].DirectChilTei = tei;
    ble_neighbor_list[tei].NeighborPcoTEI = pcotei;
    ble_neighbor_list[tei].Layer = ble_neighbor_list[pcotei].Layer + 1;
    OS_CRITICAL_EXIT();
    return true;
}
//删除pctei下的tei直接子站点
//mfg = 0:搜索删除,1不搜索直
bool ble_del_tei_from_pco(uint16_t tei,uint16_t pcotei)
{
    uint16_t priorbrothertei = 0;
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    if(true == ble_find_tei_is_pco_child(tei,pcotei,&priorbrothertei))
    {//搜索到，删除链表
        if(priorbrothertei == 0)//无上一兄弟节点，为链表头
        {
            ble_neighbor_list[pcotei].DirectChilTei = ble_neighbor_list[tei].BrotherNextTei;
        }
        else
        {
            ble_neighbor_list[priorbrothertei].BrotherNextTei = ble_neighbor_list[tei].BrotherNextTei;
        }
    }
    else
    {//无搜索到
        //rt_kprintf("%d\t !!!DelTeiNoInPcoChil ERR Tei:%d\tPtei:%d\trPtei:%d\r\n",rt_tick_get(),tei,pcotei,ble_neighbor_list[tei].NeighborPcoTEI);
        while(1);
    }
    ble_neighbor_list[tei].BrotherNextTei = 0;
    ble_neighbor_list[tei].NeighborPcoTEI = 0x00;
    ble_neighbor_list[tei].Layer = 0x00;
    OS_CRITICAL_EXIT();
    //CCO_PRINTF("{%d\tC\tLP\tTei=%d\tPco=%d}\r\n",rt_tick_get(),tei,pcotei);
    return true;
}
//485表列表处理
//功能：查找tei是否在pcotei直接子站点，若在返回ture,prior返回上一跳兄弟节点
//false不在pcotei下
bool ble_find_m485_tei_is_pco(uint16_t tei,uint16_t pcotei,uint16_t *prior)
{
    uint16_t pointtei = ble_neighbor_list[pcotei].Meter485ListNextTei;
    uint16_t priortei = 0;
    while(pointtei != 0)
    {
        if(pointtei == tei)
        {
            if(prior != NULL)
            {
                *prior = priortei;
            }
            return true;
        }
        priortei = pointtei;
        pointtei = ble_neighbor_list[pointtei].Meter485ListNextTei;
    }
    return false;
}
//把tei插入pcotei直接子站点下
bool ble_insert_m485_tei_to_pco(uint16_t tei,uint16_t pcotei)
{
    //CCO_PRINTF("{%d\tC\tJP\tTei=%d\tPco=%d\tOPco=%d}\r\n",rt_tick_get(),tei,pcotei,ble_neighbor_list[tei].NeighborPcoTEI);
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    ble_neighbor_list[tei].Meter485ListNextTei = ble_neighbor_list[pcotei].Meter485ListNextTei;
    ble_neighbor_list[pcotei].Meter485ListNextTei = tei;
    ble_neighbor_list[tei].NeighborPcoTEI = pcotei;
    ble_neighbor_list[tei].Layer = ble_neighbor_list[pcotei].Layer + 1;
    ble_neighbor_list[tei].DirectChilTei = 0x00;
    ble_neighbor_list[tei].BrotherNextTei = 0x00;
    OS_CRITICAL_EXIT();
    return true;
}
//删除pctei下的tei直接子站点
bool ble_del_m485_tei_from_pco(uint16_t tei,uint16_t pcotei)
{
    uint16_t priortei = 0;
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    if(true == ble_find_m485_tei_is_pco(tei,pcotei,&priortei))
    {
        if(priortei == 0)//无上一兄弟节点，为链表头
        {
            ble_neighbor_list[pcotei].Meter485ListNextTei = ble_neighbor_list[tei].Meter485ListNextTei;
        }
        else
        {
            ble_neighbor_list[priortei].Meter485ListNextTei = ble_neighbor_list[tei].Meter485ListNextTei;

        }
    }
    else
    {
        //rt_kprintf("%d\t !!!DelTeiNoInPcoChil ERR 485Tei:%d\tPtei:%d\trPtei:%d\r\n",rt_tick_get(),tei,pcotei,ble_neighbor_list[tei].NeighborPcoTEI);
    }

    ble_neighbor_list[tei].NeighborPcoTEI = 0x00;
    ble_neighbor_list[tei].DirectChilTei = 0x00;
    ble_neighbor_list[tei].BrotherNextTei = 0x00;
    ble_neighbor_list[tei].Meter485ListNextTei = 0;
    ble_neighbor_list[tei].Layer = 0x00;
    OS_CRITICAL_EXIT();
    //CCO_PRINTF("{%d\tC\tLP\tTei=%d\tPco=%d}\r\n",rt_tick_get(),tei,pcotei);
    return true;
}

//获取pcotei节点下的所有子站点,层级从前到后存放
uint16_t ble_get_tei_list_from_pco_tei(uint16_t *teilist,uint16_t pcotei)
{
    uint16_t teisum = 0;
    int writeindex = 0;
    int nextrindex = 0;
    
    uint16_t rparenttei = pcotei;
    do{
        uint16_t chiltei = ble_neighbor_list[rparenttei].DirectChilTei;
        while(chiltei != 0x00)
        {
            teilist[writeindex++] = chiltei;//放置到列表中
            ++teisum;
            chiltei = ble_neighbor_list[chiltei].BrotherNextTei;//下一个子节点
        }
        if(writeindex > nextrindex)
        {
            rparenttei = teilist[nextrindex];
            ++nextrindex;
        }
        else
        {
            break;
        }
        if(BLE_MAX_NEIGHBOR_NUM <= teisum)
        {
            break;
        }
    }while(true);//读取与写入相同，代表已便历空          
    return teisum;
}
//获取pcotei节点下的所有直连子站点
//teilist最大1015个
uint16_t ble_get_dirt_tei_list_from_pco_tei(uint16_t *teilist,uint16_t pcotei)
{
    int nextrindex = 0;
    uint16_t chiltei = ble_neighbor_list[pcotei].DirectChilTei;
    while(chiltei != 0x00)
    {
        if(teilist != NULL)
        teilist[nextrindex] = chiltei;//放置到列表中
        chiltei = ble_neighbor_list[chiltei].BrotherNextTei;//下一个子节点
        nextrindex++;
    }
    return nextrindex;
}
//获取tei路由表列表；
//datalen 单位是uint16_t的个数
uint16_t ble_get_pco_tei_chil_route_brush_data(uint16_t pcotei,uint16_t *pdatalist,uint16_t *datalen,uint16_t* dirtteisum,uint16_t* dirtpcosum)
{
    uint16_t pcochilchilteisum = 0;
    uint16_t dirtstasum = 0;
    uint16_t dirtpcoteisum = 0;
    uint16_t writedatalistindex = 0;
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    uint16_t teipcochillistsum = ble_get_dirt_tei_list_from_pco_tei(DataTeiList,pcotei);
    uint16_t chiltei = 0;
    for(int i=0;i<teipcochillistsum;i++)
    {
        chiltei = DataTeiList[i];
        if(ble_neighbor_list[chiltei].DirectChilTei == 0x00)//sta
        {
            pdatalist[writedatalistindex++] = chiltei;
            ++dirtstasum;
        }
    }
    
    for(int i=0;i<teipcochillistsum;i++)
    {
        chiltei = DataTeiList[i];
        if(ble_neighbor_list[chiltei].DirectChilTei != 0x00)//pco
        {
            pdatalist[writedatalistindex++] = chiltei;
            uint16_t pcochilteisum = ble_get_tei_list_from_pco_tei(&pdatalist[writedatalistindex + 1],chiltei);
            pcochilchilteisum += pcochilteisum;
            pdatalist[writedatalistindex++] = pcochilteisum;
            writedatalistindex += pcochilteisum;
            
            ++dirtpcoteisum;
        }
    }
    OS_CRITICAL_EXIT();
    *dirtteisum = dirtstasum;
    *dirtpcosum = dirtpcoteisum;
    *datalen = (dirtstasum + (dirtpcoteisum<<1) + pcochilchilteisum)*sizeof(uint16_t);
    return 1;
}


//获取到达指定tei的下一跳路由tei,0失败；其它为nexttei
uint16_t ble_get_to_tei_route(uint16_t tei)
{
    uint16_t nexttei = 0;
    uint16_t pcotei = tei;
	if(tei == BLE_CCO_TEI)return BLE_CCO_TEI;
    if(((tei>= BLE_MIN_TEI_VALUE)&&(tei<= BLE_MAX_TEI_VALUE))
    &&(ble_neighbor_list[tei].NeightState == NEIGHT_HAVE_FLG_VALUE)
    &&(ble_neighbor_list[tei].NeighborPcoTEI != 0)
    )
    {
        for(int i=0;i <= BLE_MAX_LAYER_VALUE;i++)
        {
            if(ble_neighbor_list[pcotei].NeighborPcoTEI == BLE_CCO_TEI)
            {
                nexttei = pcotei;
                break;
            }
            else
            {
                pcotei = ble_neighbor_list[pcotei].NeighborPcoTEI;
            }
        }
    }
    return nexttei;
}



//通过macaddr地址tei获取；
uint16_t ble_get_mac_addr_to_net_tei(uint8_t *macaddr)
{
	uint16_t tei = 0;
    int insertindex = ble_sreach_mac_addr_list(macaddr,ble_system_params.MacAddrList,ble_system_params.MacAddrSum,NULL);
    if(-1 != insertindex)
    {
        uint16_t neighborindex = ble_system_params.MacAddrList[insertindex].NeightIndex;
        if(neighborindex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
        {
            if(ble_neighbor_list[neighborindex].NeightState != NEIGHT_HAVE_FLG_VALUE)return false;//未分配的，异常
            tei = neighborindex;
        }
    }
    return tei;
}

void ble_net_set_neighbor(uint8_t *macaddr,uint8_t rssi)
{
	if(macaddr != NULL)
	{
		CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
		ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
		uint16_t macaddrsum = ble_system_params.MacAddrSum;
		int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
		if(-1 != mlidx)
		{//在档案内，可以删除
			ble_mac_addr_type *macaddrinfo = &ble_system_params.MacAddrList[mlidx];
			uint16_t neighborindex = macaddrinfo->NeightIndex;
			if(neighborindex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
			{
				ble_neighbor_list[neighborindex].RSSI = rssi;
				if(ble_neighbor_list[neighborindex].CurrentRxBeaconCnt < 200)
				{
					ble_neighbor_list[neighborindex].CurrentRxBeaconCnt += 1;
				}
			}
		}
		OS_CRITICAL_EXIT();
	}
}

//获取到达指定tei的路径，目<-源 返回TEI数量，0为非入网；
uint16_t ble_get_to_tei_route_path(uint16_t to_tei,uint16_t *path_list)
{
	uint16_t path_idx = 0;
    uint16_t pcotei = to_tei;
    if(((to_tei>= BLE_MIN_TEI_VALUE)&&(to_tei<= BLE_MAX_TEI_VALUE))
    &&(ble_neighbor_list[to_tei].NeightState == NEIGHT_HAVE_FLG_VALUE)
    &&(ble_neighbor_list[to_tei].NeighborPcoTEI != 0)
    )
    {
		path_list[path_idx++] = to_tei;
        for(int i=0;i <= BLE_MAX_LAYER_VALUE;i++)
        {
            if(ble_neighbor_list[pcotei].NeighborPcoTEI == BLE_CCO_TEI)
            {
				path_list[path_idx++] = BLE_CCO_TEI;
                break;
            }
            else
            {
                pcotei = ble_neighbor_list[pcotei].NeighborPcoTEI;
				path_list[path_idx++] = pcotei;
            }
        }
    }
    return path_idx;
}




//===========================================
//tei离网，且下接所有节点将离网；
//0成功，其它失败；
uint16_t ble_leave_pco_tei(uint16_t tei)//离网pco tei,若tei为pco，则teipco下的所有子节点全部离网
{
    uint16_t delpcosum = 0;
    uint16_t del485msum = 0;
    uint16_t delstasum = 0;
    uint16_t teipcochillistsum = 0;

    if((tei < BLE_MIN_TEI_VALUE)||(tei > BLE_MAX_TEI_VALUE))
    {
        while(1);
    }

    uint16_t pcotei = ble_neighbor_list[tei].NeighborPcoTEI;
    if(pcotei == 0x00)return 0;
    //
    //MAC_PRINTF("%d\tLeave Net tei%d\r\n",rt_tick_get(),tei);
    //删除子节点
    if(ble_neighbor_list[tei].DirectChilTei != 0x00)//若为pco节点，将删除pco下面的所有节点；
    {
        delpcosum++;//自身pco
        CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
        teipcochillistsum = ble_get_tei_list_from_pco_tei(DataTeiList,tei);
        for(int i=0;i<teipcochillistsum;i++)
        {//所有子站点从后到前退出离网
            uint16_t chiltei = DataTeiList[teipcochillistsum - 1 - i];//从后到前
            //uint16_t chilpco = ble_neighbor_list[chiltei].NeighborPcoTEI;
            if(ble_neighbor_list[chiltei].DirectChilTei != 0x00)
            {//pco
                delpcosum++;
            }
            else
            {
                delstasum++;
            }
            //I采或II采,删除485表
            uint16_t m485tei = ble_neighbor_list[chiltei].Meter485ListNextTei;
            while(m485tei != 0)
            {
                ble_del_m485_tei_from_pco(m485tei,chiltei);
                if(ble_neighbor_list[m485tei].NetState == NET_IN_NET)
                {
                    ble_set_tei_out_net_stay_state(m485tei);//stayBiteMap,离线
                }
                ble_neighbor_list[m485tei].NetState = NET_OUT_NET;//离网
                #ifdef FIND_BIT_MAP_TST
                if((HplcNetFindTeiBitMap[m485tei>>3]&(0x01<<(m485tei&0x07))) != 0x00)
                {
                    HplcNetFindTeiBitMap[m485tei>>3] &= ~(0x01<<(m485tei&0x07));
                }
                else
                {
                    rt_kprintf("%d\t!!!LeaveList 485tei:%d\r\n",rt_tick_get(),m485tei);
                    while(1);
                }
                #endif
                m485tei = ble_neighbor_list[chiltei].Meter485ListNextTei;
                del485msum++;
            }
            //删除tei本身
            //CCO_PRINTF("{%d\tC\tLP\tTei=%d\tPco=%d}\r\n",rt_tick_get(),chiltei,ble_neighbor_list[chiltei].NeighborPcoTEI);
            ble_neighbor_list[chiltei].NeighborPcoTEI = 0x00;
            ble_neighbor_list[chiltei].DirectChilTei = 0x00;
            ble_neighbor_list[chiltei].BrotherNextTei = 0x00;
            ble_neighbor_list[chiltei].Meter485ListNextTei = 0;
            ble_neighbor_list[chiltei].Layer = 0x00;
            //
            if(ble_neighbor_list[chiltei].NetState == NET_IN_NET)
            {
                ble_set_tei_out_net_stay_state(chiltei);//stayBiteMap,离线
            }
            ble_neighbor_list[chiltei].NetState = NET_OUT_NET;//离网
            #ifdef FIND_BIT_MAP_TST
            if((HplcNetFindTeiBitMap[chiltei>>3]&(0x01<<(chiltei&0x07))) != 0x00)
            {
                HplcNetFindTeiBitMap[chiltei>>3] &= ~(0x01<<(chiltei&0x07));
            }
            else
            {
                rt_kprintf("%d\t!!!LeaveList chiltei:%d\r\n",rt_tick_get(),chiltei);
                while(1);
            }
            #endif
        }
        OS_CRITICAL_EXIT();
    }
    else
    {
        delstasum++;//自身sta
    }
    //===============
    //I采或II采,删除485表
    uint16_t m485tei = ble_neighbor_list[tei].Meter485ListNextTei;
    while(m485tei != 0)
    {
        ble_del_m485_tei_from_pco(m485tei,tei);
        if(ble_neighbor_list[m485tei].NetState == NET_IN_NET)
        {
            ble_set_tei_out_net_stay_state(m485tei);//stayBiteMap,离线
        }
        ble_neighbor_list[m485tei].NetState = NET_OUT_NET;//离网
        #ifdef FIND_BIT_MAP_TST
        if((HplcNetFindTeiBitMap[m485tei>>3]&(0x01<<(m485tei&0x07))) != 0x00)
        {
            HplcNetFindTeiBitMap[m485tei>>3] &= ~(0x01<<(m485tei&0x07));
        }
        else
        {
            rt_kprintf("%d\t!!!LeaveNet 485tei:%d\r\n",rt_tick_get(),m485tei);
            while(1);
        }
        #endif
        m485tei = ble_neighbor_list[tei].Meter485ListNextTei;
        del485msum++;
    }
    //删除自身
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    ble_del_tei_from_pco(tei,pcotei);//从兄弟链及PCO中表中删除,肯定有父节，及兄弟节点
    ble_neighbor_list[tei].DirectChilTei = 0;//子节点链表去除
    OS_CRITICAL_EXIT();
    
    if(ble_neighbor_list[tei].NetState == NET_IN_NET)
    {
        //MAC_PRINTF("%d\tLeave OutNet tei%d\r\n",rt_tick_get(),tei);
        ble_set_tei_out_net_stay_state(tei);//离线
    }
    ble_neighbor_list[tei].NetState = NET_OUT_NET;//离网
    #ifdef FIND_BIT_MAP_TST
    if((HplcNetFindTeiBitMap[tei>>3]&(0x01<<(tei&0x07))) != 0x00)
    {
        HplcNetFindTeiBitMap[tei>>3] &= ~(0x01<<(tei&0x07));
    }
    else
    {
        rt_kprintf("%d\t!!!LeaveNet tei:%d\r\n",rt_tick_get(),tei);
        while(1);
    }
    #endif
    OS_CRITICAL_ENTER();
    ble_net_info.HpclInNetSum -= del485msum+delstasum+delpcosum;
    ble_net_info.HplcPcoSum -= delpcosum;
    if((pcotei != BLE_CCO_TEI)&&(ble_neighbor_list[pcotei].DirectChilTei == 0x00))
    {//旧父节点变更为sta；
        ble_neighbor_list[pcotei].Role = STA_ROLE;
        --ble_net_info.HplcPcoSum;
    }
    OS_CRITICAL_EXIT();
    return 0;
}

//tei入网一个pco tei
//0成功，非0失败；
//mofe 0:正常
//mofe 1:刷新入网;
uint16_t ble_into_pco_tei(uint16_t mode,uint16_t tei,uint16_t pcotei,uint16_t dtype,uint16_t phaseline)//加入pco,tei为sta或是pco;新加入或变更代理
{
    uint16_t teipcochillistsum = 0;
    uint16_t oldpcotei = ble_neighbor_list[tei].NeighborPcoTEI;
    uint16_t oldnetstate = NET_OUT_NET;
    //uint16_t selectpco = 0;
    s16 layerdiff = 0;
    //
    if((tei < BLE_MIN_TEI_VALUE)||(tei > BLE_MAX_TEI_VALUE))return INTO_NET_TEI_RANG_ERR;
    if((pcotei < BLE_MIN_TEI_VALUE)||(pcotei > BLE_MAX_TEI_VALUE))return INTO_NET_PCO_RANG_ERR;
    if((pcotei != BLE_CCO_TEI)&&(ble_neighbor_list[pcotei].NeighborPcoTEI == 0))return INTO_NET_PCO_NOT_IN_NET_ERR;
    //
    if(tei > ble_net_info.HplcMaxTei)ble_net_info.HplcMaxTei = tei;
    //
	{
		if(oldpcotei != 0)
		{
			if(pcotei != oldpcotei)
			{
				if(ble_neighbor_list[oldpcotei].ResDirtChilSum > 0)ble_neighbor_list[oldpcotei].ResDirtChilSum++;
				ble_neighbor_list[oldpcotei].ResDirtChilSum++;
			}
		}
		
		uint16_t pco_list_idx = ble_neighbor_list[tei].ResPcoTeiListIdx;
		if(pco_list_idx >= BLE_NEIGHT_RES_PCO_LIST_LEN)
		{
			pco_list_idx = 0;
		}
		ble_neighbor_list[tei].ResPcoTeiList[pco_list_idx] = pcotei;//备选pcotei列表
		ble_neighbor_list[tei].ResPcoTeiListIdx = pco_list_idx;

	}
    //MAC_PRINTF("%d\tJoin Net Tei:%d\tPco:%d\tOPco:%d\r\n",rt_tick_get(),tei,pcotei,oldpcotei);
    
    if(dtype != METER_485)//节点入网
    {
        //==========485离网
        if(oldpcotei != 0)
        {//已入网
            //从485表转换成sta节点，必然删除
            if(ble_neighbor_list[tei].DeviceType == METER_485)
            {//原来是485,将父节点中退出485
                ble_del_m485_tei_from_pco(tei,oldpcotei);//退出485表入网
                if(ble_neighbor_list[tei].NetState == NET_IN_NET)
                {
                    ble_set_tei_out_net_stay_state(tei);//stayBiteMap,离线
                }
                ble_neighbor_list[tei].NetState = NET_OUT_NET;//离网
                #ifdef FIND_BIT_MAP_TST
                if((HplcNetFindTeiBitMap[tei>>3]&(0x01<<(tei&0x07))) != 0x00)
                {
                    HplcNetFindTeiBitMap[tei>>3] &= ~(0x01<<(tei&0x07));
                }
                else
                {
                    rt_kprintf("%d\t!!!InNet Leave 485 to sta tei:%d\r\n",rt_tick_get(),tei);
                    while(1);
                }
                #endif
                ble_neighbor_list[tei].DirectChilTei = 0;
                ble_neighbor_list[oldpcotei].Role = STA_ROLE;
                //
                CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
                --ble_net_info.HpclInNetSum;
                OS_CRITICAL_EXIT();
            }
            else
            {
                //==========网络节点离网
                if(oldpcotei == pcotei)
                {
                    ble_neighbor_list[tei].DeviceType =   dtype;
                    return INTO_NET_REQ_SUCCESS;//加入同一个pco，不再处理，代表成功；
                }
                if(ble_neighbor_list[pcotei].Layer >= BLE_MAX_LAYER_VALUE)return INTO_NET_MAX_LEVEL_FAIL;//层欠超过15
                if(pcotei == tei)return INTO_NET_RING_FAIL;//自己为自己的父节点，回环
                bool pcochflg = false;
                if((pcotei == BLE_CCO_TEI)||(ble_neighbor_list[pcotei].DirectChilTei != 0x00))
                {//ok
                    pcochflg = true;
                }
                if(pcochflg == false)
                {
                    if(ble_net_info.HplcPcoSum < BLE_NET_MAX_PCO_SUM)
                    {
                        pcochflg =true;
                    }
                    else
                    {
                        if(mode == 1)
                        {//直接刷新
                            pcochflg = true;//返向刷新
                        }
                        else
                        {
                            uint16_t dirtpcostasum = ble_get_dirt_tei_list_from_pco_tei(NULL,oldpcotei);
                            if(dirtpcostasum <= 1)
                            {
                                //if((ble_net_info.HplcPcoSum - 1) < BLE_NET_MAX_PCO_SUM)
                                {
                                    pcochflg = true;
                                }
                            }
                        }
                    }
                }
                if(pcochflg == false)return INTO_NET_MAX_PCOS_FAIL;
                if(ble_neighbor_list[tei].DirectChilTei != 0x00)//tei是pco 节点,判断pcotei是否为自己的子节点
                {
                    pcochflg = true;
                    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
                    teipcochillistsum = ble_get_tei_list_from_pco_tei(DataTeiList,tei);
                    for(int i=0;i<teipcochillistsum;i++)
                    {
                        if(DataTeiList[i] == pcotei)
                        {
                            pcochflg = false;//回环,变更失败
                            break;
                        }
                    }
                    OS_CRITICAL_EXIT();
                    if(pcochflg == false)
                    {
                        return INTO_NET_REQ_CHILD_FAIL;
                    }
                }
                //网络节点,先离线
                oldnetstate = ble_neighbor_list[tei].NetState;
                layerdiff = ble_neighbor_list[tei].Layer;
                ble_del_tei_from_pco(tei,oldpcotei);//TEI及下接子节点暂退出网络树
                CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
                if((oldpcotei != BLE_CCO_TEI)&&(ble_neighbor_list[oldpcotei].DirectChilTei == 0x00))
                {//旧父节点变更为sta；
                    ble_neighbor_list[oldpcotei].Role = STA_ROLE;
                    --ble_net_info.HplcPcoSum;
                }
                --ble_net_info.HpclInNetSum;
                OS_CRITICAL_EXIT();
                
                if(ble_neighbor_list[tei].NetState == NET_IN_NET)
                {
                    ble_set_tei_out_net_stay_state(tei);//离线
                }
                ble_neighbor_list[tei].NetState = NET_OUT_NET;//离网
                #ifdef FIND_BIT_MAP_TST
                if((HplcNetFindTeiBitMap[tei>>3]&(0x01<<(tei&0x07))) != 0x00)
                {
                    HplcNetFindTeiBitMap[tei>>3] &= ~(0x01<<(tei&0x07));
                }
                else
                {
                    rt_kprintf("%d\t!!!InNet Leave tei:%d\r\n",rt_tick_get(),tei);
                    while(1);
                }
                #endif
            }
        }
        else
        {//新入网
            if((pcotei != BLE_CCO_TEI)&&(ble_neighbor_list[pcotei].DirectChilTei == 0x00))
            {
                if(ble_net_info.HplcPcoSum >= BLE_NET_MAX_PCO_SUM)
                {
                    return INTO_NET_MAX_PCOS_FAIL;
                }
            }
        }
        //==============
        //入网操作
        CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
        if((pcotei != BLE_CCO_TEI)&&(ble_neighbor_list[pcotei].DirectChilTei == 0x00))
        {//产生新的代理pco
            ble_neighbor_list[pcotei].Role = PCO_ROLE;
            ++ble_net_info.HplcPcoSum;
        }
        ble_insert_tei_to_pco(tei,pcotei);//父节点子节点列表中
        ble_net_info.HpclInNetSum++;
        OS_CRITICAL_EXIT();
        ble_neighbor_list[tei].Phase = phaseline;
        ble_neighbor_list[tei].NetState = NET_OFFLINE_NET;//离网
        #ifdef FIND_BIT_MAP_TST
        if((HplcNetFindTeiBitMap[tei>>3]&(0x01<<(tei&0x07))) == 0x00)
        {
            HplcNetFindTeiBitMap[tei>>3] |= (0x01<<(tei&0x07));
        }
        else
        {
            rt_kprintf("%d\t!!!InNet Find tei:%d\r\n",rt_tick_get(),tei);
            while(1);
        }
        #endif
        if(oldnetstate == NET_IN_NET)
        {
            ble_set_tei_in_net_stay_state(tei);
        }
        //=======================
        //tei为pco节点
        if(ble_neighbor_list[tei].DirectChilTei != 0x00)//有子点
        {//刷新子节点
            layerdiff = ble_neighbor_list[tei].Layer - layerdiff;
            //uint16_t chiltei = ble_net_info.ble_neighbor_list[tei].DirectChilTei;
            //if(ble_net_info.ble_neighbor_list[chiltei].Layer != ble_net_info.ble_neighbor_list[tei].Layer+1)
            {//加入不同层级，将重刷新层次号
                CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
                teipcochillistsum = ble_get_tei_list_from_pco_tei(DataTeiList,tei);
                for(int i=0;i<teipcochillistsum;i++)
                {
                    uint16_t chiltei = DataTeiList[teipcochillistsum - 1 - i];
                    //uint16_t chilpcotei = ble_neighbor_list[chiltei].NeighborPcoTEI;
                    uint16_t chillayer = ble_neighbor_list[chiltei].Layer + layerdiff;
                    if(chillayer > BLE_MAX_LAYER_VALUE)
                    {//超过层级离网
                        uint16_t del485msum = 0;
                        //I采或II采,删除485表
                        uint16_t m485tei = ble_neighbor_list[chiltei].Meter485ListNextTei;
                        while(m485tei != 0)
                        {
                            ble_del_m485_tei_from_pco(m485tei,chiltei);
                            if(ble_neighbor_list[m485tei].NetState == NET_IN_NET)
                            {
                                ble_set_tei_out_net_stay_state(m485tei);//stayBiteMap,离线
                            }
                            ble_neighbor_list[m485tei].NetState = NET_OUT_NET;//离网
                            #ifdef FIND_BIT_MAP_TST
                            if((HplcNetFindTeiBitMap[m485tei>>3]&(0x01<<(m485tei&0x07))) != 0x00)
                            {
                                HplcNetFindTeiBitMap[m485tei>>3] &= ~(0x01<<(m485tei&0x07));
                            }
                            else
                            {
                                rt_kprintf("%d\t!!!LeaveNet 485tei:%d\r\n",rt_tick_get(),m485tei);
                                while(1);
                            }
                            #endif
                            ble_neighbor_list[m485tei].DirectChilTei = 0x00;
                            ble_neighbor_list[m485tei].Role = STA_ROLE;
                            m485tei = ble_neighbor_list[chiltei].Meter485ListNextTei;
                            del485msum++;
                        }
                        //删除tei本身
                        if((chiltei != BLE_CCO_TEI)&&(ble_neighbor_list[chiltei].DirectChilTei != 0x00))
                        {//本节点变更为sta
                            ble_neighbor_list[chiltei].Role = STA_ROLE;
                            --ble_net_info.HplcPcoSum;
                        }
                        
                        //ble_del_tei_from_pco(chiltei,chilpcotei);//16层有父节点，小于16层可能无父节点
                        //CCO_PRINTF("{%d\tC\tLP\tTei=%d\tPco=%d}\r\n",rt_tick_get(),chiltei,ble_neighbor_list[chiltei].NeighborPcoTEI);

                        ble_neighbor_list[chiltei].NeighborPcoTEI = 0x00;
                        ble_neighbor_list[chiltei].DirectChilTei = 0x00;
                        ble_neighbor_list[chiltei].BrotherNextTei = 0x00;
                        ble_neighbor_list[chiltei].Meter485ListNextTei = 0;
                        ble_neighbor_list[chiltei].Layer = 0x00;

                        
                        if(ble_neighbor_list[chiltei].NetState == NET_IN_NET)
                        {
                            ble_set_tei_out_net_stay_state(chiltei);//stayBiteMap,离线
                        }
                        ble_neighbor_list[chiltei].NetState = NET_OUT_NET;//离网
                        #ifdef FIND_BIT_MAP_TST
                        if((HplcNetFindTeiBitMap[chiltei>>3]&(0x01<<(chiltei&0x07))) != 0x00)
                        {
                            HplcNetFindTeiBitMap[chiltei>>3] &= ~(0x01<<(chiltei&0x07));
                        }
                        else
                        {
                            rt_kprintf("%d\t!!!LeaveNet chiltei:%d\r\n",rt_tick_get(),chiltei);
                            while(1);
                        }
                        #endif
                        ble_net_info.HpclInNetSum -= del485msum+1;//485表及自己
                    }
                    else
                    {
                        ble_neighbor_list[chiltei].Layer = chillayer;
                        ble_neighbor_list[chiltei].Phase = phaseline;
                        if(chillayer == BLE_MAX_LAYER_VALUE)
                        {//15层
                            if((chiltei != BLE_CCO_TEI)&&(ble_neighbor_list[chiltei].DirectChilTei != 0x00))
                            {//本节点变更为sta
                                ble_neighbor_list[oldpcotei].Role = STA_ROLE;
                                --ble_net_info.HplcPcoSum;
                            }
                            ble_neighbor_list[chiltei].DirectChilTei = 0x00;
                        }
                    }
                }
                OS_CRITICAL_EXIT();
                if((tei != BLE_CCO_TEI)&&(ble_neighbor_list[tei].Layer == BLE_MAX_LAYER_VALUE))
                {//本节点变更为sta
                    ble_neighbor_list[tei].Role = STA_ROLE;
                    --ble_net_info.HplcPcoSum;
                    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
                    ble_neighbor_list[tei].DirectChilTei = 0x00;
                    OS_CRITICAL_EXIT();
                }
            }
        }
        else
        {
            ble_neighbor_list[tei].Role = STA_ROLE;
        }
        //网络状态刷新
        if(oldpcotei == 0)
        {//新入网节点
            //uint16_t CheckPhaseResult = 0;
            //SetNeighborInfoExtMember(tei,(uint8_t*)&CheckPhaseResult,GetMemberOff(ST_NEIGHBOR_INFO_EXTRA,CheckPhaseResult),GetMemberSize(ST_NEIGHBOR_INFO_EXTRA,CheckPhaseResult));
            //uint16_t CheckPhaseCnt = 0;
            //SetNeighborInfoExtMember(tei,(uint8_t*)&CheckPhaseCnt,GetMemberOff(ST_NEIGHBOR_INFO_EXTRA,CheckPhaseCnt),GetMemberSize(ST_NEIGHBOR_INFO_EXTRA,CheckPhaseCnt));

            ble_neighbor_list[tei].CollectMeterListFlg = 0;//
            ble_neighbor_list[tei].Role = STA_ROLE;
            //ble_neighbor_info_st_type *pnbInfo = &ble_neighbor_list[tei];
        }
    }
    else
    {//当前为入网485节点入网
        if(oldpcotei != 0)
        {
            if(ble_neighbor_list[tei].DeviceType != METER_485)
            {//原来是sta或pco,退出网络，重入485
                ble_leave_pco_tei(tei);
                ble_insert_m485_tei_to_pco(tei,pcotei);//父节点子节点列表中
                ble_neighbor_list[tei].Role = STA_ROLE;//UNKNOWN_ROLE;
                ble_neighbor_list[tei].NetState = NET_OFFLINE_NET;//离网
                ++ble_net_info.HpclInNetSum;
                #ifdef FIND_BIT_MAP_TST
                if((HplcNetFindTeiBitMap[tei>>3]&(0x01<<(tei&0x07))) == 0x00)
                {
                    HplcNetFindTeiBitMap[tei>>3] |= (0x01<<(tei&0x07));
                }
                else
                {
                    rt_kprintf("%d\t!!!InNet Find tei:%d\r\n",rt_tick_get(),tei);
                    while(1);
                }
                #endif
                ble_set_tei_in_net_stay_state(tei);
                ble_neighbor_list[tei].DirectChilTei = 0x00;
            }
            else
            {//同样是485
                if(oldpcotei != pcotei)
                {
                    ble_del_m485_tei_from_pco(tei,oldpcotei);//退出原来采集器
                    ble_insert_m485_tei_to_pco(tei,pcotei);//父节点子节点列表中
                    ble_neighbor_list[tei].Role = UNKNOWN_ROLE;
                }
            }
        }
        else
        {
            ble_insert_m485_tei_to_pco(tei,pcotei);//父节点子节点列表中
            ble_neighbor_list[tei].Role = STA_ROLE;//UNKNOWN_ROLE;
            ble_neighbor_list[tei].NetState = NET_OFFLINE_NET;//离线
            ++ble_net_info.HpclInNetSum;
            #ifdef FIND_BIT_MAP_TST
            if((HplcNetFindTeiBitMap[tei>>3]&(0x01<<(tei&0x07))) == 0x00)
            {
                HplcNetFindTeiBitMap[tei>>3] |= (0x01<<(tei&0x07));
            }
            else
            {
                rt_kprintf("%d\t!!!InNet Find tei:%d\r\n",rt_tick_get(),tei);
                while(1);
            }
            #endif
            ble_set_tei_in_net_stay_state(tei);//入网
        }
    }
    ble_neighbor_list[tei].DeviceType =   dtype;
    return INTO_NET_REQ_SUCCESS;
}

//=============
//================================
//白名单管理
//白名单添加地址
bool ble_data_add_addrs(const uint8_t *p_addrs,const uint8_t count)
{
    bool rflg = true;
    for(int i=0;i<count;i++)
    {
        if(-1 == ble_insert_mac_addr_to_list(&p_addrs[i*BLE_LONG_ADDR_SIZE],00,BLE_MAC_ADDR_NORMAL_ATTR))
        {
            rflg = false;
            break;
        }
    }
	return rflg;
}
//从白名单中删除通信地址
void ble_data_del_addrs(const uint8_t *p_addrs,const uint8_t count)
{
    for(int i=0;i<count;i++)
    {
        ble_delete_mac_addr_from_list(&p_addrs[i*BLE_LONG_ADDR_SIZE],false);
    }
}
//清空白名单通信地址
void ble_data_clear_addrs()
{
	ble_clear_mac_addr_list();
}
//获取白名单中的通信地址总数
uint32_t ble_data_get_count()
{
	//ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    //uint16_t macaddrsum = ble_system_params.MacAddrSum;
	//const System_Params_Type *psparams =  GetSystemParams();
	uint16_t macaddrnormalsum = ble_system_params.MacAddrNormalSum + ble_system_params.MacAddrFindedSum;
	return macaddrnormalsum;
}
//获取白名单中的通信地址
uint32_t ble_data_get_addrs(uint8_t *p_addrs,const uint32_t offset,const uint32_t count)
{
    const ble_system_params_type *psparams =  &ble_system_params;
    uint16_t mac_addr_normal_sum = psparams->MacAddrNormalSum + psparams->MacAddrFindedSum;
    uint16_t mac_addr_sum = psparams->MacAddrSum;
    
    uint16_t need_read_meter_start_index = offset;
    uint16_t need_read_meter_sum = count;

    if(need_read_meter_start_index >= mac_addr_normal_sum)return 0;
	
    need_read_meter_sum = (mac_addr_normal_sum - need_read_meter_start_index) > need_read_meter_sum?need_read_meter_sum:(mac_addr_normal_sum - need_read_meter_start_index);

    //
	uint16_t mac_addr_read_idx = 0;
	uint16_t mac_addr_read_cnt = 0;
    if(need_read_meter_sum > 0)
    {
        for(int i=0;i<mac_addr_sum;i++)
        {
            if((psparams->MacAddrList[i].MacAttr == BLE_MAC_ADDR_NORMAL_ATTR)
				||(psparams->MacAddrList[i].MacAttr == BLE_MAC_ADDR_FINDED_ATTR))
            {
                if(mac_addr_read_idx >= need_read_meter_start_index)
                {
                    memcpy(p_addrs,psparams->MacAddrList[i].MacAddr,BLE_LONG_ADDR_SIZE);
                    p_addrs += BLE_LONG_ADDR_SIZE;
					mac_addr_read_cnt++;
					if(mac_addr_read_cnt >= need_read_meter_sum)
					{
						break;
					}
                }
                ++mac_addr_read_idx;
            }
        }
    }
	return mac_addr_read_cnt;
}
//获取mac地址到知地址
uint16_t ble_get_mac_to_tei(uint8_t *macaddr)
{
	uint16_t tei = 0;
    ble_mac_addr_type *macaddrlist = ble_system_params.MacAddrList;
    uint16_t macaddrsum = ble_system_params.MacAddrSum;
    int mlidx = ble_sreach_mac_addr_list(macaddr,macaddrlist,macaddrsum,NULL);
    if(-1 != mlidx)
	{
		if(macaddrlist[mlidx].NeightIndex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
		{
			tei = macaddrlist[mlidx].NeightIndex;
		}
	}
	return tei;
}


//通过tei获取macaddr地址；
bool ble_get_net_tei_mac_addr(uint16_t tei,uint8_t *macaddr)
{
    //memset(macaddr, 0xff, BLE_LONG_ADDR_SIZE);
    if((tei < BLE_MIN_TEI_VALUE) || (tei > BLE_MAX_TEI_VALUE))return false;
    if(ble_neighbor_list[tei].NeightState != NEIGHT_HAVE_FLG_VALUE)return false;//未分配的，异常
	memcpy(macaddr, ble_neighbor_list[tei].MacAddr, BLE_LONG_ADDR_SIZE);
    //GetNeighborInfoExtMember(tei,macaddr,GetMemberOff(ST_NEIGHBOR_INFO_EXTRA,Mac),LONG_ADDR_SIZE);
    return true;
}

//==========
//拓扑
//获取网络拓扑节点总数
uint32_t ble_data_get_topo_node_count()
{
	return ble_net_info.HpclStayNetSum + 1;
}

//获取网络拓扑节点
uint32_t ble_data_get_topo_nodes(ble_data_topo_node_info_t *p_nodes,const uint32_t offset, const uint32_t count)
{
    uint16_t mac_node_sum = ble_system_params.MacAddrSum + 1;
    
    uint16_t need_read_node_start_index = offset;
    uint16_t need_read_node_sum = count;

    if(need_read_node_start_index >= mac_node_sum)return 0;

     need_read_node_sum = (mac_node_sum - need_read_node_start_index) > need_read_node_sum?need_read_node_sum:(mac_node_sum - need_read_node_start_index);

	uint16_t mac_addr_node_idx = 0;
	uint16_t mac_addr_node_cnt = 0;
	
    if(need_read_node_sum > 0)
    {
		for(int i = 0;i<mac_node_sum;i++)
		{
			if(mac_addr_node_cnt>=need_read_node_sum)break;
			if(i == 0)
			{//cco
				if(mac_addr_node_idx>=need_read_node_start_index)
				{
					memcpy(p_nodes->mac,ble_system_params.SystemLongAdd,BLE_LONG_ADDR_SIZE);
					p_nodes->tei = BLE_CCO_TEI;
					p_nodes->pco_tei = 0;
					p_nodes->level = 0;
					p_nodes->role = LC_NET_CCO_ROLE;
					p_nodes->add_check_flg = lc_net_get_project_id_code()==0?0:1;//sta pco cco 
					p_nodes->red = 0;
					//
					++p_nodes;
					++mac_addr_node_cnt;
				}
				++mac_addr_node_idx;
			}
			else
			{
				uint16_t teiindex = ble_system_params.MacAddrList[i - 1].NeightIndex;
				if(teiindex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
				{
					ble_neighbor_info_st_type *nodeinfo = &ble_neighbor_list[teiindex];
					if(nodeinfo->NeightState == NEIGHT_HAVE_FLG_VALUE)
					{
						if((nodeinfo->NeighborPcoTEI != 0x00)&&(nodeinfo->NetState == NET_IN_NET))
						{
							if(mac_addr_node_idx>=need_read_node_start_index)
							{
								memcpy(p_nodes->mac,ble_system_params.MacAddrList[i - 1].MacAddr,BLE_LONG_ADDR_SIZE);
								p_nodes->tei = teiindex;
								p_nodes->pco_tei = nodeinfo->NeighborPcoTEI;
								p_nodes->level = nodeinfo->Layer;
								p_nodes->role = ((nodeinfo->Role)&0x0F);
								p_nodes->add_check_flg = nodeinfo->pro_check_flg;
								p_nodes->red = 0;
								//
								++p_nodes;
								++mac_addr_node_cnt;
							}
							++mac_addr_node_idx;
						}
					}
				}
			}
		}
	}
    //
    return mac_addr_node_cnt;
}
//===============================================================================
//邻居
//获取网络拓扑节点总数
uint32_t ble_data_get_neighbor_node_count()
{
    const ble_system_params_type *psparams =  &ble_system_params;
    u16 macaddrsum = psparams->MacAddrSum;
	return macaddrsum;
}

//获取网络拓扑节点
uint32_t ble_data_get_neighbor_node_info(ble_data_neighbor_node_info_t *p_nodes,const uint32_t offset, const uint32_t count)
{
    const ble_system_params_type *psparams =  &ble_system_params;
    u16 macaddrsum = psparams->MacAddrSum;
    
    u16 needreadmeterindex = offset;
    u16 needreadmetersum = count;
    
    if(needreadmeterindex < macaddrsum)
    {//0个
        needreadmetersum = (needreadmeterindex + needreadmetersum) >= macaddrsum?(macaddrsum-needreadmeterindex):needreadmetersum;
    }
    else
    {
        needreadmetersum = 0;
    }

    for(int nidx = 0;nidx<needreadmetersum;nidx++)
    {
//        if((psparams->MacAddrList[needreadmeterindex].MacAttr == BLE_MAC_ADDR_RELAY_ATTR)
//        ||(psparams->MacAddrList[needreadmeterindex].MacAttr == BLE_MAC_ADDR_NORMAL_ATTR))
        {
            memcpy(p_nodes[nidx].mac,psparams->MacAddrList[needreadmeterindex].MacAddr,BLE_LONG_ADDR_SIZE);
            if(psparams->MacAddrList[needreadmeterindex].NeightIndex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
            {
                u16 tei = psparams->MacAddrList[needreadmeterindex].NeightIndex;
                const ble_neighbor_info_st_type *pnbninfo = &ble_neighbor_list[tei];
                //
				u16 pcotei = pnbninfo->NeighborPcoTEI;
                if((pcotei != 0)&&((pnbninfo->CurrentRxBeaconCnt) > 0))
                {
					p_nodes[nidx].rssi = pnbninfo->RSSI;
				}
				else
				{
					p_nodes[nidx].rssi = 0;
				}
                p_nodes[nidx].red = 0;
            }
            else
            {//没入过网
				p_nodes[nidx].rssi = 0;
                p_nodes[nidx].red = 0;
            }
        }
        needreadmeterindex++;
    }
	return needreadmetersum;
}
//================================================================================
//local neighbor
#define BRUSH_LOCAL_NEIGHTBOR_BRUSH_TIME_MS		(3 * 60 * 1000UL)  //3分钟
#define BLE_LOCAL_NEIGHBOR_LIST_LEN				(120)
typedef struct
{
   __VOLATILE uint8_t MacAddr[BLE_LONG_ADDR_SIZE];  //MAC
   __VOLATILE uint8_t rssi;
   __VOLATILE uint8_t rx_cnt;
}ble_local_neighbor_info_st_type;
typedef struct
{
	uint32_t ble_local_neighbor_len;
	TimeValue ble_local_neighbor_list_brush_timer;
	ble_local_neighbor_info_st_type ble_local_neighbor_list[BLE_LOCAL_NEIGHBOR_LIST_LEN];
}ble_local_neighbor_info_type;

ble_local_neighbor_info_type ble_local_neighbor_info;
//==
void ble_local_neighbor_list_init()
{
	memset(&ble_local_neighbor_info,0,sizeof(ble_local_neighbor_info));
	StartTimer(&ble_local_neighbor_info.ble_local_neighbor_list_brush_timer,BRUSH_LOCAL_NEIGHTBOR_BRUSH_TIME_MS);
}

bool ble_local_set_neighbor_info(uint8_t *mac_addr,uint8_t rssi)
{
	bool r_flg = false;
	uint32_t insert_idx = ble_local_neighbor_info.ble_local_neighbor_len;
	int8_t insert_rssi = 0;
	uint32_t check_mode = 0;//rssi
	//
	uint32_t n_idx = 0;
	for(n_idx=0;n_idx<ble_local_neighbor_info.ble_local_neighbor_len;n_idx++)
	{
		if(0 == memcmp(ble_local_neighbor_info.ble_local_neighbor_list[n_idx].MacAddr,mac_addr,BLE_LONG_ADDR_SIZE))
		{
			insert_idx = n_idx;
			r_flg = true;
			break;
		}
		else
		{
			if(check_mode == 0)
			{
				if(ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt == 0)
				{
					insert_idx = n_idx;
					check_mode = 1;
				}
				else
				{
					int8_t n_rssi = (int8_t)(ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rssi);
					if(n_rssi >= 0)
					{
						n_rssi = 0;
					}
					else
					{
						n_rssi = (0 - n_rssi);
					}
					if(insert_rssi <= n_rssi)
					{
						insert_rssi = n_rssi;
						insert_idx = n_idx;
					}
				}
			}
		}
	}
	
	if(r_flg == true)
	{
		ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rssi = rssi;
		ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt = 1;
	}
	else
	{
		if(check_mode == 1)
		{
			//replace the one whose rx_cnt == 0, and ble_local_neighbor_len not change
			memcpy(ble_local_neighbor_info.ble_local_neighbor_list[insert_idx].MacAddr,mac_addr,BLE_LONG_ADDR_SIZE);
			ble_local_neighbor_info.ble_local_neighbor_list[insert_idx].rssi = rssi;
			ble_local_neighbor_info.ble_local_neighbor_list[insert_idx].rx_cnt = 1;
			//ble_local_neighbor_info.ble_local_neighbor_len++;
			r_flg = true;
		}
		else
		{
			if(ble_local_neighbor_info.ble_local_neighbor_len < BLE_LOCAL_NEIGHBOR_LIST_LEN)
			{
				insert_idx = ble_local_neighbor_info.ble_local_neighbor_len;
			}
			if((insert_idx + 1) < BLE_LOCAL_NEIGHBOR_LIST_LEN)
			{
				memcpy(ble_local_neighbor_info.ble_local_neighbor_list[n_idx].MacAddr,mac_addr,BLE_LONG_ADDR_SIZE);
				ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rssi = rssi;
				ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt = 1;
				ble_local_neighbor_info.ble_local_neighbor_len++;
				r_flg = true;
			}
		}
	}
	//
	return r_flg;
}

void ble_local_neighbor_list_task()
{
	if(LcTimeOut(&ble_local_neighbor_info.ble_local_neighbor_list_brush_timer))
	{
		/*
		uint32_t p_idx = 0;
		uint32_t p_sum = ble_local_neighbor_info.ble_local_neighbor_len;
		uint32_t n_idx = 0;
		uint32_t n_sum = ble_local_neighbor_info.ble_local_neighbor_len;
		//
		for(p_idx=0; n_idx < p_sum; n_idx++)
		{
			if(ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt == 0)
			{
				memmove(&(ble_local_neighbor_info.ble_local_neighbor_list[n_idx]),
						&(ble_local_neighbor_info.ble_local_neighbor_list[n_idx + 1]),
						(n_sum - 1) * sizeof(ble_local_neighbor_info_st_type));
				ble_local_neighbor_info.ble_local_neighbor_len --;
			}
			else
			{
				n_idx++;
                ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt = 0;
			}
			n_sum--;
		}
		*/
		for(int n_idx = 0; n_idx < ble_local_neighbor_info.ble_local_neighbor_len; )
		{
			if(ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt == 0)
			{
				memmove(&(ble_local_neighbor_info.ble_local_neighbor_list[n_idx]),
						&(ble_local_neighbor_info.ble_local_neighbor_list[n_idx + 1]),
						(ble_local_neighbor_info.ble_local_neighbor_len - n_idx - 1) * sizeof(ble_local_neighbor_info_st_type));
				ble_local_neighbor_info.ble_local_neighbor_len --;
			}
			else
			{
                ble_local_neighbor_info.ble_local_neighbor_list[n_idx].rx_cnt = 0;
				n_idx++;
			}
		}
        StartTimer(&ble_local_neighbor_info.ble_local_neighbor_list_brush_timer, BRUSH_LOCAL_NEIGHTBOR_BRUSH_TIME_MS);
	}
}

//邻居
//获取网络拓扑节点总数
uint32_t ble_local_neighbor_get_count()
{
	return ble_local_neighbor_info.ble_local_neighbor_len;
}

//获取网络拓扑节点
uint32_t ble_local_get_neighbor_node_info(ble_data_neighbor_node_info_t *p_nodes,const uint32_t offset, const uint32_t count)
{
    u16 macaddrsum = ble_local_neighbor_info.ble_local_neighbor_len;
    
    u16 needreadmeterindex = offset;
    u16 needreadmetersum = count;
    
    if(needreadmeterindex < macaddrsum)
    {//0个
        needreadmetersum = (needreadmeterindex + needreadmetersum) >= macaddrsum?(macaddrsum-needreadmeterindex):needreadmetersum;
    }
    else
    {
        needreadmetersum = 0;
    }

    for(int nidx = 0;nidx<needreadmetersum;nidx++)
    {
		memcpy(p_nodes[nidx].mac,ble_local_neighbor_info.ble_local_neighbor_list[needreadmeterindex].MacAddr,BLE_LONG_ADDR_SIZE);
		p_nodes[nidx].rssi = ble_local_neighbor_info.ble_local_neighbor_list[needreadmeterindex].rssi;
		p_nodes[nidx].red = 0;
        needreadmeterindex++;
    }
	return needreadmetersum;
}

void addr6_memcpy(void *des, const void *src)
{
    memcpy(des, src, 6);
}

//================================================================================
//离线指示
//检测发送离线指示
uint16_t ble_get_leave_net_addr_process(uint8_t *maclist,uint16_t maxaddrlen,uint16_t *plidx)
{
    uint16_t addrsum = 0;
    if(ble_system_params.MacAddrSum <= 0)return 0;
    uint16_t idx = *plidx;
    uint16_t i=0;
    for(i = 0; i < ble_system_params.MacAddrSum; i++)
    {//回收档案
        if(idx>= ble_system_params.MacAddrSum)idx=0;
        if((ble_system_params.MacAddrList[idx].MacAttr == BLE_MAC_ADDR_NEED_DELETE_ATTR)
        &&(ble_system_params.MacAddrList[idx].NeightIndex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE))
        {
            if(maclist != NULL)
            {
                addr6_memcpy(maclist,ble_system_params.MacAddrList[idx].MacAddr);
                maclist += BLE_LONG_ADDR_SIZE;
            }
            ++addrsum;
            if(addrsum >= maxaddrlen)break;
        }
        idx++;
    }
    *plidx = idx;
    return addrsum;
}

//检测发送离线指示
uint16_t ble_get_leave_net_tei_process(uint8_t *teilist,uint16_t teiaddrlen,uint16_t *plidx)
{
    uint16_t addrsum = 0;
    if(ble_system_params.MacAddrSum <= 0)return 0;
    uint16_t idx = *plidx;
    uint16_t i=0;
    for(i = 0; i < ble_system_params.MacAddrSum; i++)
    {//回收档案
        if(idx>= ble_system_params.MacAddrSum)idx=0;
        if((ble_system_params.MacAddrList[idx].MacAttr == BLE_MAC_ADDR_NEED_DELETE_ATTR)
        &&(ble_system_params.MacAddrList[idx].NeightIndex != BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE))
        {
            if(teilist != NULL)
            {
                //addr6_memcpy(teilist,ble_system_params.MacAddrList[idx].MacAddr);
				uint16_t tei = ble_system_params.MacAddrList[idx].NeightIndex;
				teilist[0] = tei&0xFF;
				teilist[1] = (tei>>8)&0xFF;
                teilist += BLE_SHORT_ADDR_SIZE;
            }
            ++addrsum;
            if(addrsum >= teiaddrlen)break;
        }
        idx++;
    }
    *plidx = idx;
    return addrsum;
}


//==================
//网络状态刷新
void ble_brush_neighbor_heart(uint16_t tei,uint16_t mode)
{
	if(tei <= BLE_CCO_TEI)return;
	if(tei > BLE_MAX_TEI_VALUE)return;
	
	if(ble_neighbor_list[tei].NeightState == NEIGHT_HAVE_FLG_VALUE)//有效的,有父节点
	{
		if(ble_neighbor_list[tei].NeighborPcoTEI != 0x00)
		{
			//
			switch(mode)
			{
				case 0://heart
					ble_neighbor_list[tei].HeartFrameHeartCnt++;          //上一周期接收到邻居信标接收数			
					break;
				case 1://neight
					ble_neighbor_list[tei].NeighborHeartCnt++;          //上一周期接收到邻居信标接收数			
					break;
				default://other
					ble_neighbor_list[tei].OtherHeartCnt++;   
					break;
			}
			//
            OS_ERR err;
			ble_neighbor_list[tei].brush_neighbor_tick = OSTimeGet(&err);
			ble_neighbor_list[tei].NetStatePeriodCount = 0;
			if(ble_neighbor_list[tei].NetState == NET_OFFLINE_NET)
			{
				ble_set_tei_in_net_stay_state(tei);
			}
		}
	}
}

///
#define  	STA_PERIOD_MAX_CNT						(15)
#define  	STA_OFFLINE_PERIOD_MAX_CNT				(3)
#define 	STA_DEL_TEI_OFFLINE_PERIOD_MAX_CNT      (8)

void ble_neighbor_heart_period_process(uint16_t addr_idx,uint16_t ngh_idx)
{
	if(ble_neighbor_list[ngh_idx].NetStatePeriodCount == 0)
	{
		ble_neighbor_list[ngh_idx].StayInNetPrioCnt++;
	}
	
	if(ble_neighbor_list[ngh_idx].NetStatePeriodCount < STA_PERIOD_MAX_CNT)
	{
		++(ble_neighbor_list[ngh_idx].NetStatePeriodCount);
	}
	if(ble_neighbor_list[ngh_idx].NetStatePeriodCount <= 1)
	{//两个周期内
		ble_neighbor_list[ngh_idx].CurrentRxBeaconCnt = 1;
	}
	else
	{//三个周期以上无心跳
		ble_neighbor_list[ngh_idx].CurrentRxBeaconCnt = 0;
	}
	
	if(ble_neighbor_list[ngh_idx].NetStatePeriodCount > STA_OFFLINE_PERIOD_MAX_CNT)
	{
		ble_leave_pco_tei(ngh_idx);//离网处理
		#ifdef BLE_NODE_IN_LEAVE_NET_REPORT_USING
		if(ble_neighbor_list[ngh_idx].NeighborPcoTEI == 0)
		{//离网,确认删除处理
			ble_neighbor_list[ngh_idx].NeedNetReport = 1;
			ble_neighbor_list[ngh_idx].NetReportFlg = 0;//初始离网已上报
		}
		#endif
	}
	#ifndef BLE_NODE_IN_LEAVE_NET_REPORT_USING
	if(ble_neighbor_list[ngh_idx].NetStatePeriodCount > STA_DEL_TEI_OFFLINE_PERIOD_MAX_CNT)
	{//回收TEI
		CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
		if((ble_system_params.MacAddrList[addr_idx].MacAttr == BLE_MAC_ADDR_NEED_DELETE_ATTR)
			|| (ble_system_params.MacAddrList[addr_idx].MacAttr == BLE_MAC_ADDR_FINDED_ATTR))
		{
			ble_leave_pco_tei(ngh_idx);//离网处理
			ble_free_tei_and_mac_addr(NULL,ngh_idx);
		}
		OS_CRITICAL_EXIT();
	}
	#endif
}
//=================
void ble_neighbor_list_period_process()
{
   //uint16_t i;
    uint16_t ngh_idx;
	int macsum = ble_system_params.MacAddrSum;
    //HplcNetParam.LocalMaxNeightSum = 0;
    for(int i = macsum-1;i>=0;i--)
    {
		//数据转存(当前路由周期结束后，把接收到的包数转为上个路由周期)       
		ngh_idx = ble_system_params.MacAddrList[i].NeightIndex;
		if(ngh_idx == BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE)
		{
			if((ble_system_params.MacAddrList[i].MacAttr == BLE_MAC_ADDR_NEED_DELETE_ATTR)
				||(ble_system_params.MacAddrList[i].MacAttr == BLE_MAC_ADDR_FINDED_ATTR))
			{
				ble_delete_mac_addr_from_list(ble_system_params.MacAddrList[i].MacAddr,true);
			}
		}
		else
		{
			//
			if((BLE_MIN_TEI_VALUE >= ngh_idx) || (ngh_idx > BLE_MAX_TEI_VALUE))continue;
			if(ble_neighbor_list[ngh_idx].DeviceType == METER_485)continue;//485表，不处理

			//ble_neighbor_info_st_type *pnbInfo = &ble_neighbor_list[ngh_idx];
			//
			ble_neighbor_heart_period_process(i,ngh_idx);
			//
			ble_neighbor_list[ngh_idx].HeartFrameHeartCnt = 0;	
			ble_neighbor_list[ngh_idx].NeighborHeartCnt = 0;		
			ble_neighbor_list[ngh_idx].OtherHeartCnt = 0;
			//
		}
	}
}


void ble_data_reset_net_data()
{
	
    CPU_SR_ALLOC();
    OS_CRITICAL_ENTER();
    //beacon slot value
    memset(&ble_neighbor_list,0,sizeof(ble_neighbor_list));
	memset(&ble_net_info,0x00,sizeof(ble_net_info));
	
    //  dsp_memset(&NeighborList,0x00,sizeof(NeighborList));
    //  dsp_memset(&NetPeriodPara,0x00,sizeof(NetPeriodPara));

    //重新组网,释放所有tei
    for(int i=0;i<ble_system_params.MacAddrSum;i++)
    {
        ble_system_params.MacAddrList[i].NeightIndex = BLE_DEFAULT_NEIGHTBOR_INDEX_TEI_VALUE;
    }

    //HplcNetParam.BeaconPriodCnt = 2; //信标周期记数
    memcpy(ble_net_info.CCO_MAC,ble_system_params.SystemLongAdd,BLE_LONG_ADDR_SIZE);


    //===========
    ble_system_params.MacAddrSum = 0;
    ble_system_params.MacAddrNormalSum = 0;
    ble_system_params.MacAddrFindedSum = 0;
    ble_system_params.MacAddrNeedDelSum = 0;
    ble_system_params.MacAddrRelaySum = 0;


    ble_net_info.NetId = ble_system_params.NetId;
	ble_net_info.NetBindFlg = 0;
    ble_net_info.NetSeq = ble_system_params.NetSeq;
	ble_net_info.NetEnableFlag = ble_system_params.NetEnableFlag;
	ble_net_info.NetModeFlg = 0;//NET_CCO_MODE;
	
    //============================
    uint16_t neightindex = BLE_CCO_TEI;//cco
    //HplcNetParam.MacAddrList[insertindex].NeightIndex = neightindex;
    //dsp_memset(&HplcNetParam.NeighborList[neightindex],0x00,sizeof(HplcNetParam.NeighborList[neightindex]));
    ble_neighbor_list[neightindex].NeightState = NEIGHT_HAVE_FLG_VALUE; //已分配占用
	memcpy(ble_neighbor_list[neightindex].MacAddr,ble_system_params.SystemLongAdd,BLE_LONG_ADDR_SIZE);
    //NeighborList[neightindex].TEI = neightindex;

    ble_neighbor_list[neightindex].Phase = 0;
    //HplcNetParam.NeighborList[neightindex].un_Role_Phase_Layer.byte_fm.Layer = 0;
    ble_neighbor_list[neightindex].DeviceType = 0x02;
    ble_neighbor_list[neightindex].NeighborPcoTEI = 0;
    ble_neighbor_list[neightindex].Role = CCO_ROLE;
    ble_neighbor_list[neightindex].NetState = NET_IN_NET; //cco 入网状态
	//
	ble_net_info.IspKey = lc_net_get_factory_id_code();
	//key开关打开时，同步给蓝牙的项目key为0
	if(ble_system_params.key_switch == 1)
	{
		ble_net_info.ProjKey = 0;
	}
	else
	{
		ble_net_info.ProjKey = lc_net_get_project_id_code();
	}
	
    //
    ble_system_params.SystemParamsStatus = PARAM_MAC_SYNC_COMPLETE_LIST_STATUS;//已同步完成
    //
    OS_CRITICAL_EXIT();
	//
	

}
