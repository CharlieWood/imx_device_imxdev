/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/*******************************************************************************
* Title: audioNullapi
*
* Filename: audioNullapi.h
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
#ifndef _AUDIO_NULL_API_H_
#define _AUDIO_NULL_API_H_

#include "hwatypes.h"

extern HWA_ComponentHandle HWNullHandle;

typedef enum
{
	NULL_SIMPLE = 0,
	NULL_TOTAL_VALID_PATHS //Must the be last one
} NullPaths;

#endif /*_AUDIO_NULL_API_H_*/
