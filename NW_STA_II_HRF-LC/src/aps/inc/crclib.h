#ifndef __CRCLIB_H__
#define __CRCLIB_H__
#include <stdint.h>

#define CRC8_INIT_VAL   0xFF
#define CRC16_INIT_VAL  0xFFFF
#define CRC24_INIT_VAL  0x0
#define CRC32_INIT_VAL  0xFFFFFFFF

#define CRC8_POLY       0x8C //crc8 polynome, Poly = x8+x5+x4+1
//#define CRC8_POLY       0xD9 //crc8 polynome, Poly = x8+x7+x4+x3+x+1

//#define CRC16_POLY      0xA001 //crc16 polynome, Poly = x16+x15+x2+1 (IBM,SDLC)
#define CRC16_POLY      0x8408 //crc16 polynome, Poly = x16+x12+x5+1 (CCITT,ISO,HDLC,ITUX25,PPP-FCS)

#define CRC24_POLY      0xC60001    

#define CRC32_POLY      0xEDB88320 //crc32 polynome, Poly = x32+x26+x23+...+x2+x+1 (ZIP,RAR,IEEE,LAN/FDDI,PPP-FCS)
//#define CRC32_POLY      0x82F63B78 //crc32 polynome, Poly = x32+x28+x27+...+x8+x6+1 (SCTP)

// uint16_t CRC16_CYC_CAL(uint16_t crc, uint8_t *data, uint32_t len);
// uint32_t CRC32_CYC_CAL(uint32_t crc, uint8_t *data, uint32_t len);

// #define CRC16(data, len) (0xFFFF ^ CRC16_CYC_CAL(0xFFFF, (data), (len)))
// #define CRC32(data, len) (0xFFFFFFFF ^ CRC32_CYC_CAL(0xFFFFFFFF, (data), (len)))

uint8_t crc8_cal(uint8_t* data, uint32_t len);
uint16_t crc16_cal(uint8_t* data, uint32_t len);
uint32_t crc24_cal(uint8_t* data, uint32_t len);
uint32_t crc32_cal(uint8_t* data, uint32_t len);

uint8_t crc8_cyc_cal(uint8_t init_val, uint8_t* data, uint32_t len);
uint16_t crc16_cyc_cal(uint16_t init_val, uint8_t* data, uint32_t len);
uint32_t crc24_cyc_cal(uint32_t init_val, uint8_t* data, uint32_t len);
uint32_t crc32_cyc_cal(uint32_t init_val, uint8_t* data, uint32_t len);
void crc_table_init(void);

#endif
