#include "soft_check.h"
#include "system_inc.h"
#define BAK_SOFT_SIZE    60// 存储的是备案文件的厂商和软件信息
static bool softUpdateFlag;
static char * code_start=NULL;
#pragma segment=".vectors"
#pragma section="ROM_CONTRNT"
extern char _etext;
//获取软件是否更新
void GetSoftUpdateFlag(void)
{
	if (code_start!=NULL)
	{
		return ;
	}
	//P_FLASH_INFO_FIELD pFlash=(P_FLASH_INFO_FIELD)FLASH_AHB_ADDR;
	P_FLASH_USER_CHARACTER_FIELD pUser1 = (P_FLASH_USER_CHARACTER_FIELD)(DATA_FLASH_SECTION_SIZE*1+FLASH_AHB_ADDR);
	P_FLASH_USER_CHARACTER_FIELD pUser2 = (P_FLASH_USER_CHARACTER_FIELD)(DATA_FLASH_SECTION_SIZE*2+FLASH_AHB_ADDR);
	if (pUser2->character==0x0123abcd
		|| pUser1->version>1) 
	{
		softUpdateFlag=true;
		code_start=(char*)0x10000000;
	}
	else
	{
		#if defined(__ICCARM__)
		uint32_t code_end = __section_size("ROM_CONTRNT")+__section_size(".vectors");
		#elif defined(__GNUC__)
		uint32_t code_end = (uint32_t)&_etext;
		#endif
		softUpdateFlag=false;
		code_start = (char*)(code_end + pUser1->code_addr +FLASH_AHB_ADDR +BAK_SOFT_SIZE);
	}
}
const char *mcu_name="WTZ13C";
int GetSoftInfo(char *data_array)
{
	soft_info_s * p=(soft_info_s*)data_array;
	memset(data_array,0,sizeof(soft_info_s));
	GetSoftUpdateFlag();
	if (softUpdateFlag)
	{
		p->cpu_num=1;
		p->factor=*(uint16_t*)c_Tmn_ver_info.factoryCode;
		p->factor = ntohs(p->factor);
		p->day = c_Tmn_ver_info.softReleaseDate.day;
		p->mon = c_Tmn_ver_info.softReleaseDate.month;
		p->year = c_Tmn_ver_info.softReleaseDate.year+2000;
		p->hard_ver = 0x0100;
#if 1
        p->soft_ver = Hex2BcdChar(c_Tmn_ver_info.software_v[0])<<8 | Hex2BcdChar(c_Tmn_ver_info.software_v[1]);
#else
        p->soft_ver= *(uint16_t*)c_Tmn_ver_info.software_v;
		p->soft_ver = ntohs(p->soft_ver);
#endif        
		p->mcu_len = strlen(mcu_name);
		if (p->mcu_len>60)
		{
			p->mcu_len=60;
		}
		memcpy(&data_array[11],mcu_name,p->mcu_len);
		return p->mcu_len+11;
	}
	else
	{
		char * data= code_start-BAK_SOFT_SIZE;
		p->cpu_num=*data++;
		p->factor=*(uint16_t*)data;
		p->factor = ntohs(p->factor);
		data+=2;
		p->day = *data++;
		p->mon = *data++;
		p->year =2000+ *data++;
		p->hard_ver=*(uint16_t*)data;
		data+=2;
		p->soft_ver= *(uint16_t*)data;
		data+=2;
		p->mcu_len = *data++;
		if (p->mcu_len>60)
		{
			p->mcu_len=60;
		}
		memcpy(&data_array[11],data,p->mcu_len);
		return p->mcu_len+11;
	}
}
SHA1Context clcSha1;
void GetCodeSha1(uint32_t start_addr, u16 len, uint8_t *sha1_data)
{
	GetSoftUpdateFlag();
	SHA1Reset(&clcSha1);
	SHA1Input(&clcSha1, (const uint8_t*)(code_start+start_addr), len);
	SHA1Result(&clcSha1, sha1_data);
}

