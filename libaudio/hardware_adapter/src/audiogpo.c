/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: audiogpio
*
* Filename: audiogpio.c
*
* Target, platform: Common Platform, SW platform
*
* Authors: Nathan Ren
*
* Description: GPIO
*
* Last Updated:
*
* Notes:
******************************************************************************/

#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include "hwa.h"
#include "audiogpoapi.h"
#include "audiogpo.h"

#define LOG_TAG "HWA_GPO"
#include <utils/Log.h>

/*===============================================================*/
/*=========== Local variable definitions ========================*/
/*===============================================================*/
/*static*/ void GPOInit(unsigned char reinit);
static HWA_DigitalGain GPOPathEnable(unsigned char path, HWA_AudioVolume volume);
static HWA_DigitalGain GPOPathDisable(unsigned char path);
static HWA_DigitalGain GPOPathVolumeSet(unsigned char path, HWA_AudioVolume volume);
static HWA_DigitalGain GPOPathMute(unsigned char path, HWA_AudioMute mute);
static short GPOGetPathsStatus(char* data, short length);
static HWA_AnalogGain GPOGetPathAnalogGain(unsigned char path);
static HWA_ReturnCode GPOSetPowerMode(HWA_PowerMode mode);
//GPIO SET AND GET
static int GPOSet(const char *gpio_name, char value);
static char GPOGetSpeaker(const char *gpio_name);
//IOCtl function
static void GPOIOCtlHandle(const char *device_name, int cmd ,int val);

HWA_ComponentHandle HWGpoHandle  =
{
    GPOInit,
    GPOPathEnable,
    GPOPathDisable,
    GPOPathVolumeSet,
    GPOPathMute,
    GPOGetPathsStatus,
    GPOGetPathAnalogGain,
    GPOSetPowerMode
};

void GPOInit(unsigned char reinit)
{
	int fd = -1;

	if(reinit == 0)
	{
		fd = open (AUDIO_PA_EN, O_RDONLY);
		if (fd == (-1))
        	{
			LOGE("GPOInit: can not open %s", AUDIO_PA_EN);
            		return;
        	}
        	close(fd);

		fd = open (EAR_POP, O_RDONLY);
		if (fd == (-1))
        	{
			LOGE("GPOInit: can not open %s", EAR_POP);
            		return;
        	}
        	close(fd);

        	LOGI("GPOInit: GPO init is successful!");
	}
} /* End of GPOInit */

static HWA_DigitalGain GPOPathEnable(unsigned char path, HWA_AudioVolume volume)
{
    LOGI("GPOPathEnable: CTRL path = %d, volume = %d", path, volume);

	switch(path)
	{
		case GPO_EXT_HEADSET_AMP_CTRL:
            	{
                	GPOSet(HP_AMP_SD, GPIO_ON);
            	}
		break;

		case GPO_EXT_HEADSET_EAR_POP_CTRL:
            	{
                	GPOSet(EAR_POP, GPIO_ON);
            	}
		break;

		case GPO_EXT_SPEAKER_AMP_CTRL:
            	{
                	GPOSet(AUDIO_PA_EN, GPIO_ON);
            	}
		break;

        	case GPO_EXT_AEC_POWER_CTRL:
            	{
                	GPOIOCtlHandle(FM2010, AEC_IOC_SLEEP, AEC_WAKE_UP);
            	}
            	break;

        	case GPO_EXT_AEC_MODE_CTRL:
            	{
                	GPOIOCtlHandle(FM2010, AEC_IOC_SET_MODE, volume);
            	}
            	break;

		default:
			break;
	}

    return 0;
} /* End of GPOPathEnable */

static HWA_DigitalGain GPOPathDisable(unsigned char path)
{
	LOGI("GPOPathDisable: CTRL path = %d", path);

	switch(path)
	{
		case GPO_EXT_HEADSET_AMP_CTRL:
        	{
                	GPOSet(HP_AMP_SD, GPIO_OFF);
            	}
		break;

		case GPO_EXT_HEADSET_EAR_POP_CTRL:
            	{
                	GPOSet(EAR_POP, GPIO_OFF);
            	}
		break;

		case GPO_EXT_SPEAKER_AMP_CTRL:
            	{
                	GPOSet(AUDIO_PA_EN, GPIO_OFF);
            	}
		break;

        	case GPO_EXT_AEC_POWER_CTRL:
            	{
                	GPOIOCtlHandle(FM2010, AEC_IOC_SLEEP, AEC_SLEEP);
            	}
            	break;

        	case GPO_EXT_AEC_MODE_CTRL:
            	break;

		default:
			break;
	}

    	return 0;
} /* End of GPOPathDisable */

static HWA_DigitalGain GPOPathVolumeSet(unsigned char path, HWA_AudioVolume volume)
{
	return 0;
} /* End of GPOPathVolumeSet */


static HWA_DigitalGain GPOPathMute(unsigned char path, HWA_AudioMute mute)
{
	return 0;
} /* End of GPOPathMute */


static short GPOGetPathsStatus(char* data, short length)
{
	return 0;
} /* End of GPOGetPathsStatus */


static HWA_AnalogGain GPOGetPathAnalogGain(unsigned char path)
{
	return 0;
} /* End of GPOGetPathAnalogGain */

static HWA_ReturnCode GPOSetPowerMode(HWA_PowerMode mode)
{
    return 0;
} /* End of GPOSetPowerMode*/


// This function is to set gpio

/************************************************
 *Set gpio 
 ************************************************
 * char '1' is enable
 * char '0' is disable.
 * return 0 for ok else for fail.
 ************************************************/
static int GPOSet(const char *gpio_name, char value)
{
    int gpio_fd = -1;

    if ((gpio_fd = open(gpio_name, O_RDWR)) < 0)
    {
        LOGE("GPOSet: Can't open %s, gpio_fd: %d", gpio_name, gpio_fd);
        return -1;
    }

    if ((write(gpio_fd, &value, sizeof(char))) < 0)
    {
        LOGE("GPOSet: Can't write %s", gpio_name);
        return -1; 
    }

    close(gpio_fd);

    return 0;
}

/************************************************
 *Get gpio 
 ************************************************
 * return '1' is enable status.
 * return '0' is disable status.
 * return else for fail.
 ************************************************/
static char GPOGet(const char *gpio_name)
{
    char value;
    int gpio_fd = -1;

    if ((gpio_fd = open(gpio_name, O_RDWR)) < 0)
    {
        LOGE("GPOGet: Can't open %s, gpio_fd: %d", gpio_name, gpio_fd);
        return -1;
    }

    if ((read(gpio_fd, &value, sizeof(char))) < 0)
    {
        LOGE("GPOGet: Can't read %s", gpio_name);
        return -1; 
    }

    close(gpio_fd);

    return value;
}

/************************************************
 *IOCtl handle
 ************************************************
 * cmd: set IOCtl command
 * val: set IOCrl value
 * return true for ok, false for fail.
 ************************************************/
static void GPOIOCtlHandle(const char *device_name, int cmd, int val)
{ 
    int device_fd = -1;
    int err;

    device_fd = open(device_name, O_RDWR);
    if (device_fd < 0)
	{
		LOGE("GPOIOCtlHandle: Open %s fail, device_fd: %d", device_name, device_fd);
        return;
	}
	
    err = ioctl(device_fd, cmd, val);
	if (err < 0)
    {
        LOGE("GPOIOCtlHandle: Can't do ioctl, err = %d", err);   
        close(device_fd);      
        return;
    }

    close(device_fd);
}
