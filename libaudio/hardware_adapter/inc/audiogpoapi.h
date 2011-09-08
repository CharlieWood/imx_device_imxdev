/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/*******************************************************************************
* Title: audioGpoApi
*
* Filename: audioGpoApi.h
*
* Target, platform: Common Platform, SW platform
*
* Authors: Nathan Ren
*
* Description:
*
* Last Updated:
*
* Notes:
*******************************************************************************/
#ifndef AUDIO_GPO_API_H
#define AUDIO_GPO_API_H

/*----------- Local include files --------------------------------------------*/

#include "hwatypes.h"

extern  HWA_ComponentHandle HWGpoHandle;

typedef enum
{
	GPO_EXT_HEADSET_AMP_CTRL  =  0,  // Headset amplifier control
	GPO_EXT_SPEAKER_AMP_CTRL,       // Speaker amplifier control

	GPO_EXT_HEADSET_EAR_POP_CTRL,  // Headset ear pop control

    	GPO_EXT_AEC_POWER_CTRL,  //AEC power control
    	GPO_EXT_AEC_MODE_CTRL,  // AEC mode control

	GPO_TOTAL_VALID_PATHS, 		 // Must the be here - counting path amount
	GPO_DUMMY_PATH,  	   		 // for disabling purposes

	GPO_PATHS_ENUM_32_BIT  = 0x7FFFFFFF  //32bit enum compiling enforcement
} GPOPaths;

#endif /* AUDIO_GPO_API_H */
