
#include "bspatch.h"
#include "tuz_dec.h"
#include "string.h"
#include "lcdiff_update.h"


extern void FeedWdg(void);

// FLASH中固件地址信息初始化
extern u8 OldCodeSection;
#define hal_flash_read(A,B,C) ReadFlash(A,B,C)
#define hal_flash_write(A,B,C)  DataFlashInit(); WriteFlash(A,B,C); DataFlashClose()
#define hal_flash_erase(A,B) DataFlashInit(); EraseFlash(A,B); DataFlashClose()
#define hal_wdt_feed() FeedWdg()


uint8_t dict_buf[TUZ_DICT_SIZE + TUZ_CACHE_SIZE];

#if 0

static unsigned int crc32_cyc_cal(const unsigned int poly, unsigned int init_val, unsigned char *data, int len)
{
    unsigned int crc = init_val;

    for (int i = 0; i < len; i++) {
        crc ^= data[i];  // 每个字节与CRC异或
        // 逐位处理当前字节的8个bit
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;  // 最低位为1时异或多项式
            } else {
                crc >>= 1;  // 最低位为0直接右移
            }
        }
    }
    return crc;  
}

#else

static unsigned int crc32_table[]= {
	// 0xEDB88320 crc32 polynome, Poly = x32+x26+x23+...+x2+x+1 (ZIP,RAR,IEEE,LAN/FDDI,PPP-FCS)
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
	0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
	0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

static unsigned int crc32c_table[]= {
	// 0x82F63B78 crc32c polynome, Poly = x32+x28+x27+...+x8+x6+1 (SCTP)
	0x00000000, 0x105EC76F, 0x20BD8EDE, 0x30E349B1,
	0x417B1DBC, 0x5125DAD3, 0x61C69362, 0x7198540D,
	0x82F63B78, 0x92A8FC17, 0xA24BB5A6, 0xB21572C9,
	0xC38D26C4, 0xD3D3E1AB, 0xE330A81A, 0xF36E6F75
};

static unsigned int crc32_cyc_cal(const unsigned int poly, unsigned int crc, unsigned char *data, int len)
{
	unsigned int *crc_table;
	if (poly == 0x82F63B78)
	{
		crc_table = crc32c_table;
	}
	else
	{
		crc_table = crc32_table;
	}
	
	while (len--)
    {
        crc = crc ^ (unsigned int)*data++;              // Consume a byte at a time
        crc = (crc >> 4) ^ crc_table[crc & 0x0F]; 			// Two rounds of 4-bits
        crc = (crc >> 4) ^ crc_table[crc & 0x0F];
    }
    return crc;
}

#endif


static uint32_t flash_crc32c_cal(const unsigned int poly, uint32_t addr, uint32_t size)
{
	#define CHECK_BUF_SIZE TUZ_DICT_SIZE
    uint32_t pos = 0;
    uint32_t crc = 0xFFFFFFFF;
    uint32_t read_len = 0;
    uint32_t remain_len = 0;
    uint8_t *check_buf = dict_buf;
	debug_str(DEBUG_LOG_UPDATA,"addr = %08x  size = %d\r\n",addr,size);

	if (size > FLASH_CODE1_SIZE)
	{
		return 0;
	}
	
    while (pos < size)
    {
        read_len = CHECK_BUF_SIZE;
        remain_len = size - pos;
        if (read_len > remain_len)
        {
            read_len = remain_len;
        }
        hal_flash_read(addr + pos, check_buf, read_len);
		crc = crc32_cyc_cal(poly, crc, check_buf, read_len);
        pos += read_len;
		hal_wdt_feed();
    }
	debug_str(DEBUG_LOG_UPDATA,"data : %02x %02x %02x %02x\r\n",check_buf[read_len-4]
	,check_buf[read_len-3],check_buf[read_len-2],check_buf[read_len-1]);
    return ~crc;

#undef CHECK_BUF_SIZE
}


static int patch_old_read(struct bspatch_stream* stream, uint32_t offset, void* buffer, int length)
{
	hal_flash_read((uint32_t)stream->opaque_old_r + offset, buffer, length);
	return 0;
}

static int patch_new_write(struct bspatch_stream *stream, void *buffer, int length)
{
	hal_wdt_feed();
	DataFlashInit();
	Unprotection1MFlash();
	//EraseFlash((u32)stream->opaque_w, length);
	WriteFlash((u32)stream->opaque_w, buffer, length);
	Protection1MFlash();
	DataFlashClose();
	//hal_flash_write((uint32_t)stream->opaque_w, buffer, length);
	stream->opaque_w = (char *)stream->opaque_w + length;
	return 0;
}

static int patch_new_read(struct bspatch_stream *stream, void *buffer, int length)
{
    uint8_t *dp = buffer;

    tuz_TResult result=tuz_OK;
    tuz_TStream * tuz = stream->opaque_r;
    do {
        tuz_size_t step_size = (tuz_size_t)length;
        result = tuz_TStream_decompress_partial(tuz, dp, &step_size);

        length -= step_size;
        dp += step_size;

    } while (length > 0 && result == tuz_OK);

    return 0;
}

static tuz_BOOL tuz_data_read(void* listener,tuz_byte* out_code,tuz_size_t* code_size) 
{
	uint32_t *tuz_stream = listener;
	hal_flash_read(*tuz_stream, out_code, *code_size);
	*tuz_stream += *code_size;
	return tuz_TRUE;
}

extern void DoUpdate(u32 tryTime);
extern void setIsOwnFileFlag();

int lcdiff_update(uint32_t download_addr, uint32_t download_area_size)
{
	debug_str(DEBUG_LOG_UPDATA,"lcdiff_update %d\r\n",OldCodeSection);
	tuz_TStream tuz;
	uint32_t tuz_stream;
	patch_header_t patch_head;
	struct bspatch_stream patch_stream;

	UserSection_Type userSection;
	
	// 参数合法性检查
	if((0 == download_addr) || (0 == download_area_size))
	{
		download_addr = FLASH_CODE1_ADDRESS + OldCodeSection * FLASH_CODE1_SIZE;
		download_area_size = FLASH_CODE1_SIZE;
	}

	// FLASH中固件地址信息初始化
	uint32_t FW_AREA_SIZE  = FLASH_CODE1_SIZE;
	uint32_t app_addr = 0;
	uint32_t userSection_addr;
	if(OldCodeSection == 2)
	{
		app_addr = FLASH_CODE1_ADDRESS;
		userSection_addr = FLASH_USER_FEATURE1_ADDRESS;
		debug_str(DEBUG_LOG_UPDATA,"addr1 = %08x\r\n",FLASH_CODE1_ADDRESS);
	}
	else if(OldCodeSection == 1)
	{
		app_addr = FLASH_CODE3_ADDRESS;
		userSection_addr = FLASH_USER_FEATURE3_ADDRESS;
		debug_str(DEBUG_LOG_UPDATA,"addr3 = %08x\r\n",FLASH_CODE3_ADDRESS);
	}
	else if(OldCodeSection == 0)
	{
		app_addr = FLASH_CODE2_ADDRESS;
		userSection_addr = FLASH_USER_FEATURE2_ADDRESS;
		debug_str(DEBUG_LOG_UPDATA,"addr2 = %08x\r\n",FLASH_CODE2_ADDRESS);
	}
	else 
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff err: size\r\n");
		return false;
	}
	
	uint32_t factory_addr = LC_UPDTAE_FACTORY;

	// 读取文件头匹配
	hal_flash_read(download_addr, (uint8_t *)&patch_head, sizeof(patch_header_t));
	if(0 != memcmp(patch_head.magic, "LCDIFF43", 8))
	{
		debug_str(DEBUG_LOG_UPDATA,"LCDIFF43 err\r\n");
		return false;
	}
	if (patch_head.new_size > FW_AREA_SIZE )
	{
		debug_str(DEBUG_LOG_UPDATA,"size err %d %d \r\n",patch_head.new_size,FW_AREA_SIZE);
		return false;
	}
	
	// 检验patch文件
	if(flash_crc32c_cal(LCDIFF_CRC_POLY, download_addr + sizeof(patch_header_t), patch_head.patch_size) != patch_head.patch_crc)
	{
			debug_str(DEBUG_LOG_UPDATA,"crc32c_cal err \r\n");
		return false;
	}

	// 读取当前运行APP信息
	hal_flash_read(userSection_addr, (uint8_t *)&userSection, sizeof(UserSection_Type));
	if (userSection.CodeSize != patch_head.old_size && (userSection.CodeSize/4 != (patch_head.old_size/4 +1)))
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff err: size %d %d \r\n",userSection.CodeSize,patch_head.old_size);
		return false;
	}
	debug_str(DEBUG_LOG_UPDATA,"addr = %08x\r\n",app_addr);
	// 校验当前运行APP是否符合差分升级
	if(flash_crc32c_cal(LCDIFF_CRC_POLY, app_addr, patch_head.old_size) != patch_head.old_crc)
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff err: crc\r\n");
		return false;
	}

	// 复制patch到factory分区
	uint32_t copy_size = sizeof(patch_header_t) + patch_head.patch_size;
	// 擦除，准备patch保存区域
	//flash_erase(factory_addr, copy_size);
	DataFlashInit();
	Unprotection1MFlash();
	EraseFlash(factory_addr, copy_size);
	Protection1MFlash();

	uint32_t pos = 0;
    uint32_t read_len = 0;
    uint32_t remain_len = 0;
    uint8_t *copy_buf = dict_buf;
	#define COPY_BUF_SIZE TUZ_DICT_SIZE
	
    while (pos < copy_size)
    {
        read_len = COPY_BUF_SIZE;
        remain_len = copy_size - pos;
        if (read_len > remain_len)
        {
            read_len = remain_len;
        }
        hal_flash_read(download_addr + pos, copy_buf, read_len);
		DataFlashInit();
		Unprotection1MFlash();
		WriteFlash(factory_addr + pos, copy_buf, read_len);
		Protection1MFlash();
		DataFlashClose();
		//hal_flash_write(factory_addr + pos, copy_buf, read_len);
        pos += read_len;
		hal_wdt_feed();
    }
	/*
	if(copy_size < 256)
	{
		hal_flash_read(factory_addr, copy_buf, copy_size);
	}
	else
	{
		hal_flash_read(factory_addr, copy_buf, 256);
	}
	

	//char tempbuf[300] = {0};
	//for(int i=0;i<80&&i<256;i++)
	//{
	//	sprintf(&tempbuf[i*3],"%02x ",copy_buf[i]);
	//}
	//debug_str(DEBUG_LOG_UPDATA,"buf:%s\r\n",tempbuf);
	// 擦除新固件保存区域
	//flash_erase(download_addr,patch_head.new_size);
	*/
	
	
	
	// 初始化解压缩
	tuz_stream = factory_addr + sizeof(patch_header_t);
	u32 tempdata = 0;
	hal_flash_read(tuz_stream, (u8 *)&tempdata, 4);
	debug_str(DEBUG_LOG_UPDATA,"size = %d tempdata=%08x \r\n",sizeof(patch_header_t),tempdata);
    tuz_size_t dictSize=tuz_TStream_read_dict_size(&tuz_stream, tuz_data_read);
    if (dictSize > TUZ_DICT_SIZE)
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff err: dictSize = %d > TUZ_DICT_SIZE\r\n",dictSize);
		return false;
	}

    if(tuz_OK != tuz_TStream_open(&tuz, &tuz_stream, tuz_data_read, dict_buf, (tuz_size_t)dictSize, TUZ_CACHE_SIZE))
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff err tuz_TStream_open\r\n");
		return false;
	}

	Unprotection1MFlash();
	EraseFlash(download_addr, patch_head.new_size);
	Protection1MFlash();
	DataFlashClose();

	// 差分包解包
	patch_stream.opaque_r = &tuz;
	patch_stream.read = patch_new_read;

	patch_stream.opaque_w = (uint8_t *)download_addr;
    patch_stream.write = patch_new_write;

	patch_stream.opaque_old_r = (uint8_t *)app_addr;
	patch_stream.old_read = patch_old_read;
	
	
	if (bspatch(patch_head.old_size, patch_head.new_size, &patch_stream) < 0) 
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff bspatch fail\r\n");
        return false;
    }
	

	// 校验新固件程序
	if(flash_crc32c_cal(LCDIFF_CRC_POLY, download_addr, patch_head.new_size) != patch_head.new_crc)
	{
		debug_str(DEBUG_LOG_UPDATA,"lcdiff bspatch err: crc\r\n");
		return false;
	}
	setIsOwnFileFlag();
	DoUpdate(0);
	//Delayms(1000);
	//REBOOT_SYSTEM();
	
	return true;
}
