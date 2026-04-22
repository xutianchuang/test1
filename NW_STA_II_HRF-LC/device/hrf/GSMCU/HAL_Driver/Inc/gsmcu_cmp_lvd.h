#ifndef __GSMCU_CMP_LVD_H__
#define __GSMCU_CMP_LVD_H__
#ifdef __cplusplus
extern "C"
{
#endif


#define ANA_VOLTAGE_1P8   1  
#define ANA_VOLTAGE_1P95  2
#define ANA_VOLTAGE_2P1   3
#define ANA_VOLTAGE_2P25  4
#define ANA_VOLTAGE_2P4   5
#define ANA_VOLTAGE_2P55  6
#define ANA_VOLTAGE_2P7   7
#define ANA_VOLTAGE_2P85  8
#define ANA_VOLTAGE_3     9
#define ANA_VOLTAGE_3P15  10
#define ANA_VOLTAGE_3P3   11

#define VBATDET_2P3       1
#define VBATDET_2P4       2
#define VBATDET_2P5       3
#define VBATDET_2P6       4
#define VBATDET_2P7       5
#define VBATDET_2P8       6
#define VBATDET_2P9       7
#define VBATDET_3P0       8
	
#define VPINDET_1P0       1
#define VPINDET_1P1       2
#define VPINDET_1P2       3
#define VPINDET_1P3       4
#define VPINDET_1P4       5
#define VPINDET_1P5       6
#define VPINDET_1P6       7
#define VPINDET_1P7       8
#define VPINDET_1P8       9
#define VPINDET_1P9       10
#define VPINDET_2P0       11
#define VPINDET_2P1       12
#define VPINDET_2P2       13
#define VPINDET_2P3       14
#define VPINDET_2P4       15
#define VPINDET_2P5       16


void LVD_Init(int low, int high);
void Comparer_Init(int low, int high);

#ifdef __cplusplus
}
#endif
#endif /* __GSMCU_CMP_LVD_H__ */

