/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: audioNull
*
* Filename: audioNull.c
*
* Target, platform: Common Platform, SW platform
*
* Authors: Nathan Ren
*
* Description: Null component (template and last component)
*
* Last Updated:
*
* Notes:
******************************************************************************/
#include "hwa.h"
#include "audionullapi.h"
#include "audionull.h"

static char voidFunc(void);
HWA_ComponentHandle HWNullHandle = {
				(HWAComponentInit_t)voidFunc,
				(HWAEnablePath_t)voidFunc,
				(HWADisablePath_t)voidFunc,
				(HWAVolumeSetPath_t)voidFunc,
				(HWAMutePath_t)voidFunc,
				(HWAGetPathsStatus_t)voidFunc,
				(HWAGetPathAnalogGain_t) voidFunc,
				(HWASetPowerMode_t) voidFunc};

/*----------- Local definitions ------------------------------------*/


/*----------- Local defines  ------------------------------------*/

static char voidFunc(void)
{
	return 0;
}
