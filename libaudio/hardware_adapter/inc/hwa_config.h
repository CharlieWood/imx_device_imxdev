/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: Audio codec hardware adapter config.
*
* Filename: hwa_config.h
*
* Authors: Nathan Ren
*
* Description: the device table config for different products 
*
* Last Updated:
*
* Notes:
******************************************************************************/

#ifndef HWA_CONFIG_H
#define HWA_CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif

#include "hwa.h"
/*
Product: aster
Audio Codec: sgtl5000
*/ 
const HWA_DeviceRoute deviceTable_ASTERBoard[] =
{
    /* device       route       component       path        appliance */
    /*Loudspeaker devices*/
    {HWA_LOUDSPEAKER,       HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_LOUDSPEAKER,              HWA_SPEAKER_PHONE},
    {HWA_LOUDSPEAKER,       HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_LOUDSPEAKER_AMP,          HWA_SPEAKER_PHONE},
    /*Headset devices*/
    {HWA_HP_SPEAKER,        HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_HEADPHONE,                HWA_HEADPHONE},
    {HWA_HP_SPEAKER,        HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_HEADPHONE_AMP,            HWA_HEADPHONE},
    /*Microphone*/
    {HWA_LOUD_MIC,          HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_PAD_INTERNAL_MIC,	  HWA_SPEAKER_PHONE},
    {HWA_LOUD_MIC,          HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_PAD_INTERNAL_MIC_BIAS,	  HWA_SPEAKER_PHONE},
    {HWA_LOUD_MIC,          HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_PAD_INTERNAL_MIC_GAIN,	  HWA_SPEAKER_PHONE},

    {HWA_HP_MIC,            HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_PAD_EXTERNAL_MIC,     	  HWA_HEADPHONE},
    {HWA_HP_MIC,            HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_PAD_EXTERNAL_MIC_BIAS,    HWA_HEADPHONE},
    {HWA_HP_MIC,            HWA_I2S,        SGTL5000_COMPONENT,        SGTL5000_PAD_EXTERNAL_MIC_GAIN,    HWA_HEADPHONE},
    {HWA_HP_MIC,			HWA_I2S,		SGTL5000_COMPONENT,		   SGTL5000_PAD_EXTERNAL_MIC_SWITCH,  HWA_HEADPHONE},

    /* Must be the last one! */
    {HWA_NOT_CONNECTED,     HWA_NO_ROUTE,   NULL_COMPONENT,         0,                      0},
};

/* Add new borad here*/
#define deviceTable_Borad deviceTable_ASTERBoard

#ifdef __cplusplus
}
#endif
#endif
