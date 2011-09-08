/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/*******************************************************************************
* Title: audioSGTL5000Api
*
* Filename: audioSGTL5000Api.h
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
#ifndef AUDIO_SGTL5000_API_H
#define AUDIO_SGTL5000_API_H

/*----------- Local include files --------------------------------------------*/

#include "hwatypes.h"

extern  HWA_ComponentHandle HWSGTL5000Handle;

typedef enum
{
	SGTL5000_LOUDSPEAKER = 0,
	SGTL5000_LOUDSPEAKER_AMP,
	SGTL5000_HEADSET,
	SGTL5000_HEADSET_AMP,
	SGTL5000_HEADPHONE,
	SGTL5000_HEADPHONE_AMP,
	SGTL5000_PAD_INTERNAL_MIC,
	SGTL5000_PAD_INTERNAL_MIC_BIAS,
	SGTL5000_PAD_INTERNAL_MIC_GAIN,
	SGTL5000_PAD_EXTERNAL_MIC,
	SGTL5000_PAD_EXTERNAL_MIC_BIAS,
	SGTL5000_PAD_EXTERNAL_MIC_GAIN,
	SGTL5000_PAD_EXTERNAL_MIC_SWITCH,		   //for headphone mic gpio switch
	SGTL5000_PATH_MAX_ID,
	SGTL5000_PATHS_ENUM_32_BIT  = 0x7FFFFFFF  //32bit enum compiling enforcement
} SGTL5000Paths;

#endif /* AUDIO_SGTL5000_API_H */
