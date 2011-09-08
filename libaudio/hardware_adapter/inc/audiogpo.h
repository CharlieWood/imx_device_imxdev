/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/*******************************************************************************
* Title: audioGpo
*
* Filename: audioGpo.h
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
#ifndef AUDIO_GPO_H
#define AUDIO_GPO_H

#ifdef __cplusplus
extern "C"{
#endif

#define AUDIO_PA_EN "/sys/kernel/audio_pa_en"
#define EAR_POP "/sys/kernel/ear_pop"
#define HP_AMP_SD "/sys/kernel/hp_amp_sd"
#define GPIO_ON  '1'
#define GPIO_OFF '0'

#define FM2010 "/dev/aec"
#define AEC_IOC_MAGIC  0xC1
#define AEC_IOC_SET_REGISTER            _IOWR(AEC_IOC_MAGIC, 1, int)
#define AEC_IOC_CMD_TEST                _IOWR(AEC_IOC_MAGIC, 2, int)
#define AEC_IOC_SET_MODE                _IOWR(AEC_IOC_MAGIC, 3, int)
#define AEC_IOC_RESET                   _IOWR(AEC_IOC_MAGIC, 4, int)
#define AEC_IOC_SLEEP                   _IOWR(AEC_IOC_MAGIC, 5, int)

typedef enum
{
    AEC_NORMAL_MODE = 0,
    AEC_BYPASS_MODE,
    AEC_NORMAL_HANDHELD_MODE,
    AEC_MODE_MAX
} AEC_FUNC_MODE;

typedef enum
{
    AEC_WAKE_UP = 0,
    AEC_SLEEP,
    AEC_INVALID
} AEC_SLEEP_MODE;

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif  /* AUDIO_GPO_H */
