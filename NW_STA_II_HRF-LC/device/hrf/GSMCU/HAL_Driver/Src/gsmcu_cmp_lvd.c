
#include  <gsmcu_hal.h>
#include "gsmcu_cmp_lvd.h"


void LVD_Init(int low, int high)
{
	uint32_t reg_value=
		high<<CPM_BATVDCFGR_BatVolDetHValue_Rst_Pos|
		low<<CPM_BATVDCFGR_BatVolDetLValue_Rst_Pos|
		1<<CPM_BATVDCFGR_BatVolDetRstEn_Pos;
	CPM->BATVDCFGR=(uint32_t)(1<<CPM_BATVDCFGR_TEST_Pos)|reg_value;
	CPM->BATVDCFGR=(uint32_t)(2<<CPM_BATVDCFGR_TEST_Pos)|reg_value;
	CPM->BATVDCFGR=(uint32_t)(3<<CPM_BATVDCFGR_TEST_Pos)|reg_value;

	CPM->BATVDCFGR=(uint32_t)(3<<CPM_BATVDCFGR_TEST_Pos)|reg_value|CPM_BATVDCFGR_BatVolDetRstEn_Msk;

	CPM->BATVDCFGR=reg_value;


	CPM->VBATDETCR_b.TEST=1;
	CPM->VBATDETCR_b.TEST=2;
	CPM->VBATDETCR_b.TEST=3;

	CPM->VBATDETCR_b.VBATDET_EN=1;

	CPM->VBATDETCR_b.TEST=0;
}


void Comparer_Init(int low, int high)
{
	uint32_t reg_value=
		high<<CPM_PINVDCFGR_PinVolDetHValue_Pos|
		low<<CPM_PINVDCFGR_PinVolDetLValue_Pos|
		1<<CPM_PINVDCFGR_PinVolDetLIE_Pos|
		1<<CPM_PINVDCFGR_PinVolDetHIE_Pos;
	CPM->PINVDCFGR=(uint32_t)(1<<CPM_PINVDCFGR_TEST_Pos)|reg_value;
	CPM->PINVDCFGR=(uint32_t)(2<<CPM_PINVDCFGR_TEST_Pos)|reg_value;
	CPM->PINVDCFGR=(uint32_t)(3<<CPM_PINVDCFGR_TEST_Pos)|reg_value;

	CPM->PINVDCFGR=(uint32_t)(3<<CPM_PINVDCFGR_TEST_Pos)|reg_value;

	CPM->PINVDCFGR=reg_value;

	CPM->VDETCR_b.TEST=1;
	CPM->VDETCR_b.TEST=2;
	CPM->VDETCR_b.TEST=3;

	CPM->VDETCR_b.VPINDET_EN=1;

	CPM->VDETCR_b.TEST=0;


	
}











