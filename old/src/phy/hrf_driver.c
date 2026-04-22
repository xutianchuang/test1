#include "hrf_port.h"




u16 HRF_PsduSymbol(u8 PSDU_MCS,u8 option,u16 len)
{

	u8  PSDU_ModulationMode;

	u8  PSDU_CopyNum;

	u8  PSDU_PuncMode;
	
	switch (PSDU_MCS)
	{
	case 0:
		PSDU_ModulationMode = 0; PSDU_CopyNum = 4; PSDU_PuncMode = 0; break;
	case 1:
		PSDU_ModulationMode = 0; PSDU_CopyNum = 2; PSDU_PuncMode = 0; break;
	case 2:
		PSDU_ModulationMode = 1; PSDU_CopyNum = 2; PSDU_PuncMode = 0; break;
	case 3:
		PSDU_ModulationMode = 1; PSDU_CopyNum = 1; PSDU_PuncMode = 0; break;
	case 4:
		PSDU_ModulationMode = 1; PSDU_CopyNum = 1; PSDU_PuncMode = 1; break;
	case 5:
		PSDU_ModulationMode = 2; PSDU_CopyNum = 1; PSDU_PuncMode = 0; break;
	case 6:
		PSDU_ModulationMode = 2; PSDU_CopyNum = 1; PSDU_PuncMode = 1; break;
	default:
		PSDU_ModulationMode = 0; PSDU_CopyNum = 4; PSDU_PuncMode = 0; break;
	}

	u16 PSDU_PBSize = GetPbSize(len);
	u16 PSDU_DataLenIn = PSDU_PuncMode ? (PSDU_PBSize << 3) + (PSDU_PBSize << 1) : PSDU_PBSize << 4;
	u8 PSDU_InterNum = (option == 1 | option == 2) ? (PSDU_CopyNum == 6  ? 6 :
													  PSDU_CopyNum == 4  ? 4 :
													  PSDU_CopyNum == 3  ? 6 :
													  PSDU_CopyNum == 2  ? 4 : 1) :
		(option == 3) ? (PSDU_CopyNum == 6  ? 6 :
						 PSDU_CopyNum == 4  ? 4 :
						 PSDU_CopyNum == 3  ? 6 :
						 PSDU_CopyNum == 2  ? 2 : 1) : 1;

	u8 ValidCarrierNum = (option == 1) ? 96 :
		(option == 2) ? 48 :
		(option == 3) ? 18 : 0;

	u8 PSDU_RoboUsedCarrierNum = PSDU_InterNum * (ValidCarrierNum / PSDU_InterNum);
	u8 PSDU_BPC = PSDU_ModulationMode == 0 ? 1 :
		PSDU_ModulationMode == 1 ? 2 : 4;
	s16 PSDU_OFDMNum_tmp;
	PSDU_OFDMNum_tmp = PSDU_DataLenIn * PSDU_CopyNum - PSDU_RoboUsedCarrierNum * PSDU_BPC;
	while (PSDU_OFDMNum_tmp > 0)
	{
		PSDU_OFDMNum_tmp = PSDU_OFDMNum_tmp - PSDU_RoboUsedCarrierNum * PSDU_BPC;
	}
	u16 PSDU_OFDMNum = PSDU_OFDMNum_tmp < 0 ? PSDU_DataLenIn * PSDU_CopyNum / (PSDU_RoboUsedCarrierNum * PSDU_BPC) + 1 : PSDU_DataLenIn * PSDU_CopyNum / (PSDU_RoboUsedCarrierNum * PSDU_BPC);
	return PSDU_OFDMNum ;
}
u16 HRF_PhrSymbol(u8 PHR_MCS,u8 option)
{
	u8  PHR_ModulationMode;

	u8  PHR_CopyNum;

	u8  PHR_PuncMode;

	switch (PHR_MCS)
	{
	case 0:
		PHR_ModulationMode = 0; PHR_CopyNum = 6; PHR_PuncMode = 0; break;
	case 1:
		PHR_ModulationMode = 0; PHR_CopyNum = 4; PHR_PuncMode = 0; break;
	case 2:
		PHR_ModulationMode = 0; PHR_CopyNum = 3; PHR_PuncMode = 0; break;
	case 3:
		PHR_ModulationMode = 0; PHR_CopyNum = 2; PHR_PuncMode = 0; break;
	case 4:
		PHR_ModulationMode = 1; PHR_CopyNum = 2; PHR_PuncMode = 0; break;
	case 5:
		PHR_ModulationMode = 1; PHR_CopyNum = 1; PHR_PuncMode = 0; break;
	case 6:
		PHR_ModulationMode = 1; PHR_CopyNum = 1; PHR_PuncMode = 1; break;
	default:
		PHR_ModulationMode = 0; PHR_CopyNum = 6; PHR_PuncMode = 0; break;
	}


	u16 PHR_DataLenIn = PHR_PuncMode ? (16 << 3) + (16 << 1) : 16 << 4;
	u8  PHR_InterNum = (option == 1 | option == 2) ? (PHR_CopyNum == 6  ? 6 :
													  PHR_CopyNum == 4  ? 4 :
													  PHR_CopyNum == 3  ? 6 :
													  PHR_CopyNum == 2  ? 4 : 1) :
		(option == 3) ? (PHR_CopyNum == 6  ? 6 :
						 PHR_CopyNum == 4  ? 4 :
						 PHR_CopyNum == 3  ? 6 :
						 PHR_CopyNum == 2  ? 2 : 1) : 1;
	u8 ValidCarrierNum = (option == 1) ? 96 :
		(option == 2) ? 48 :
		(option == 3) ? 18 : 0;

	u8 PHR_RoboUsedCarrierNum = PHR_InterNum * (ValidCarrierNum / PHR_InterNum);
	u8 PHR_BPC = PHR_ModulationMode == 0 ? 1 :
		PHR_ModulationMode == 1 ? 2 : 4;
	s16 PHR_OFDMNum_tmp;
	PHR_OFDMNum_tmp = PHR_DataLenIn * PHR_CopyNum - PHR_RoboUsedCarrierNum * PHR_BPC;
	while (PHR_OFDMNum_tmp > 0)
	{
		PHR_OFDMNum_tmp = PHR_OFDMNum_tmp - PHR_RoboUsedCarrierNum * PHR_BPC;
	}
	u16 PHR_OFDMNum = PHR_OFDMNum_tmp < 0 ? PHR_DataLenIn * PHR_CopyNum / (PHR_RoboUsedCarrierNum * PHR_BPC) + 1 : PHR_DataLenIn * PHR_CopyNum / (PHR_RoboUsedCarrierNum * PHR_BPC);
	return  PHR_OFDMNum;
}

u16 HRFrameLen(u8 phr,int psdu,u8 option)//100us
{
	u32 totalUs=0;
	if (phr)
	{
		//SIG
		if (option==1)
		{
			totalUs=TFFT+T_G;
		}
		else
		{
			totalUs=2*TFFT+T_G;
		}
		//STF+LTF
		totalUs+=TFFT*5+2.5*TFFT+T_TR;
		//phr
		totalUs+=(TFFT+T_G)*phr;
	}
	if (psdu)
	{
		totalUs+=(TFFT+T_G)*psdu;
	}
	return (totalUs+9999)/10000;
}



uint32_t HRF_GetSYNC_NTB(void)
{
	return BPLC->NTB_ZERO_CROSSING7;
}


extern int HRF_Reset(void);
void HRF_PhyReset(void)
{
  	HRF_Reset();
}
