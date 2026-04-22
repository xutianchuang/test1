#include <stdint.h>

#ifndef _CRC_HEAD__
#define _CRC_HEAD__

#define CRC_BASE              0x40400000UL
#define CRC_CTR               (*(volatile uint32_t *)(CRC_BASE))

#define CRC_RES               (*(volatile uint32_t *)(CRC_BASE+0x4))
#define CRC_INI               (*(volatile uint32_t *)(CRC_BASE+0x8))
#define CRC_DATI              (*(volatile uint32_t *)(CRC_BASE+0xc))

#define CRC_RES16_0           (*(volatile uint16_t *)(CRC_BASE+0x4))
#define CRC_RES16_1           (*(volatile uint16_t *)(CRC_BASE+0x6))

#define CRC_DATI16_0          (*(volatile uint16_t *)(CRC_BASE+0xc))
#define CRC_DATI16_1          (*(volatile uint16_t *)(CRC_BASE+0xe))

#define CRC_DATI8_0           (*(volatile uint8_t *)(CRC_BASE+0xc))
#define CRC_DATI8_1           (*(volatile uint8_t *)(CRC_BASE+0xd))
#define CRC_DATI8_2           (*(volatile uint8_t *)(CRC_BASE+0xe))
#define CRC_DATI8_3           (*(volatile uint8_t *)(CRC_BASE+0xf))

#define CRC_MODE32            0x80000000
#define CRC_MODE16            0x40000000
#define CRC_MODE24            0x20000000
#define CRC_UPDATE            0x10000000
#define CRC_SRCSEL            0x08000000
#define CRC_BIGEND            0x04000000
#define CRC_BYTSWAP           0x02000000
#define CRC_RDSWAP            0x01000000
#define CRC_CHKPASS           0x00000001


uint32_t Hal_Crc(uint32_t crcmode, uint8_t *pdata, uint32_t datalen);

#endif
