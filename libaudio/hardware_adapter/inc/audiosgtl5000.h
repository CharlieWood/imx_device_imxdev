/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/*******************************************************************************
* Title: audiosgtl5000
*
* Filename: audiosgtl5000.h
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
#ifndef AUDIO_SGTL5000_H
#define AUDIO_SGTL5000_H

#ifdef __cplusplus
extern "C"{
#endif
/*---------------------------------------------------------------------------*/
typedef enum
{
	PATH_DISABLE = 0,
	PATH_ENABLE
}SGTL5000_PATH_STATUS;

typedef enum
{
	GAIN_0dB = 0,
	GAIN_20dB,
	GAIN_30dB,
	GAIN_40dB
}SGTL5000_MIC_GAIN;

#define NUMID_OFFSET 6
#define MAX_LENGTH 128
#define SGTL5000_CONFIG_FILENAME "/system/etc/sgtl5000_audio_calibration_config"

typedef enum
{
	CHIP_DIG_POWER = 0,
	CHIP_CLK_CTRL,
	CHIP_I2S_CTRL,
	CHIP_SSS_CTRL,
	CHIP_CHIP_ADCDAC_CTRL,
	CHIP_DAC_VOL,
	CHIP_PAD_STRENGTH,
	CHIP_ANA_ADC_CTRL,
	CHIP_ANA_HP_CTRL,
	CHIP_ANA_CTRL,
	CHIP_LINREG_CTRL,
	CHIP_REF_CTRL,
	CHIP_MIC_CTRL,
	CHIP_LINE_OUT_CTRL,
	CHIP_LINE_OUT_VOL,
	CHIP_ANA_POWER,
	CHIP_PLL_CTRL,
	CHIP_CLK_TOP_CTRL,
	CHIP_ANA_STATUS,
	CHIP_SHORT_CTRL,
	CHIP_MAX_NUMID
}SGTL5000_REGISTER;

typedef struct
{
   const char *function_name;
   const char *function_value;
}SGTL5000_FUNCTION;

typedef struct
{
   SGTL5000_REGISTER register_id;
   short register_val;
}SGTL5000_REGISTER_DESCRIPTION;

#ifdef __cplusplus
}
#endif
#endif  /* AUDIO_SGTL5000_H */
