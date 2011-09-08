/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: audiosgtl5000
*
* Filename: audiosgtl5000.c
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
******************************************************************************/

#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <asoundlib.h>

#include "hwa.h"
#include "audiosgtl5000api.h"
#include "audiosgtl5000.h"

#define LOG_TAG "HWA_SGTL5000"
#include <utils/Log.h>

#define SGTL5000_DEBUG
#ifdef SGTL5000_DEBUG
#define HWA_SGTL5000_LOG LOGD
#else
#define HWA_SGTL5000_LOG
#endif

/*===============================================================*/
/*=========== Local variable definitions ========================*/
/*===============================================================*/
static char sgtl5000_ctl[64] = "default";

static SGTL5000_REGISTER_DESCRIPTION calibration_register_description[CHIP_MAX_NUMID + 1];

static SGTL5000_REGISTER_DESCRIPTION default_register_description[CHIP_MAX_NUMID + 1] = {
        {CHIP_DIG_POWER,	-1},
        {CHIP_CLK_CTRL,		-1},
        {CHIP_I2S_CTRL,		-1},
        {CHIP_SSS_CTRL,		-1},
        {CHIP_CHIP_ADCDAC_CTRL,	-1},
        {CHIP_DAC_VOL,		0x3C3C},
        {CHIP_PAD_STRENGTH,	-1},
        {CHIP_ANA_ADC_CTRL,	0X00FF},
        {CHIP_ANA_HP_CTRL,	0x1818},
        {CHIP_ANA_CTRL,		-1},
        {CHIP_LINREG_CTRL,	-1},
        {CHIP_REF_CTRL,		-1},
        {CHIP_MIC_CTRL,		-1},
        {CHIP_LINE_OUT_CTRL,	-1},
        {CHIP_LINE_OUT_VOL,	0x0404},
        {CHIP_ANA_POWER,	-1},
        {CHIP_PLL_CTRL,		-1},
        {CHIP_CLK_TOP_CTRL,	-1},
        {CHIP_ANA_STATUS,	-1},
        {CHIP_SHORT_CTRL,	-1},
        {CHIP_MAX_NUMID,	-1}
};


SGTL5000_FUNCTION sgtl5000_funcs_on[] = {
	{"Speaker Function",	"on"},
	{"Power Function", 	"speaker_amp_on"},
	{"Jack Function", 	"on"},
	{"Power Function", 	"headset_amp_on"},
	{"Jack Function", 	"on"},
	{"Power Function", 	"headset_amp_on"},
	{"Line In Function", 	"on"},
	{"Mic bias Function", 	"on"},
	{"MIC GAIN",		"30dB"},
	{"Line In Function", 	"on"},
	{"Mic bias Function", 	"on"},
	{"MIC GAIN",		"30dB"},
	{"Mic Switch",		"ext"},
	{"", 			""}
};

SGTL5000_FUNCTION sgtl5000_funcs_off[] = {
	{"Speaker Function", 	"off"},
	{"Power Function", 	"speaker_amp_off"},
	{"Jack Function", 	"off"},
	{"Power Function", 	"headset_amp_off"},
	{"Jack Function", 	"off"},
	{"Power Function", 	"headset_amp_off"},
	{"Line In Function", 	"off"},
	{"Mic bias Function", 	"off"},
	{"MIC GAIN",		"0dB"},
	{"Line In Function", 	"off"},
	{"Mic bias Function", 	"off"},
	{"MIC GAIN",		"0dB"},
	{"Mic Switch",		"int"},
	{"", 			""}
};

/*static*/ void SGTL5000Init(unsigned char reinit);
static HWA_DigitalGain SGTL5000PathEnable(unsigned char path, HWA_AudioVolume volume);
static HWA_DigitalGain SGTL5000PathDisable(unsigned char path);
static HWA_DigitalGain SGTL5000PathVolumeSet(unsigned char path, HWA_AudioVolume volume);
static HWA_DigitalGain SGTL5000PathMute(unsigned char path, HWA_AudioMute mute);
static short SGTL5000GetPathsStatus(char* data, short length);
static HWA_AnalogGain SGTL5000GetPathAnalogGain(unsigned char path);
static HWA_ReturnCode SGTL5000SetPowerMode(HWA_PowerMode mode);
static int SGTL5000SetFunction(SGTL5000_FUNCTION *func);
static int SGTL5000Set(snd_ctl_t *handle, const char *name, unsigned int value, int index);
static int SGTL5000WriteRegisters(unsigned short sgtl5000RegNumid, unsigned short sgtl5000RegVal);
static int SGTL5000ReadRegisters(unsigned short sgtl5000RegNumid, unsigned short *sgtl5000RegVal);
static void SGTL5000ReadCalibrationFile(void);
static void SGTL5000ReadDefaultConfigParameters(void);
static void SGTL5000ReadConfigParameters(FILE *sgtl5000_config_fd);

HWA_ComponentHandle HWSGTL5000Handle  =
{
    SGTL5000Init,
    SGTL5000PathEnable,
    SGTL5000PathDisable,
    SGTL5000PathVolumeSet,
    SGTL5000PathMute,
    SGTL5000GetPathsStatus,
    SGTL5000GetPathAnalogGain,
    SGTL5000SetPowerMode
};

void SGTL5000Init(unsigned char reinit)
{
	SGTL5000SetFunction(&sgtl5000_funcs_off[SGTL5000_LOUDSPEAKER]);
	SGTL5000SetFunction(&sgtl5000_funcs_off[SGTL5000_HEADSET]);
	SGTL5000SetFunction(&sgtl5000_funcs_off[SGTL5000_PAD_EXTERNAL_MIC]);
	SGTL5000SetFunction(&sgtl5000_funcs_off[SGTL5000_PAD_EXTERNAL_MIC_SWITCH]);
	SGTL5000ReadCalibrationFile();
	HWA_SGTL5000_LOG("SGTL5000Init:init is successful!");
} /* End of SGTL5000Init */

static HWA_DigitalGain SGTL5000PathEnable(unsigned char path, HWA_AudioVolume volume)
{
	HWA_SGTL5000_LOG("SGTL5000PathEnable: CTRL path = %d, volume = %d", path, volume);

	if(path >= SGTL5000_PATH_MAX_ID)
	{
		return -1;
	}

        if(SGTL5000SetFunction(&sgtl5000_funcs_on[path]) < 0)
	{
		return -1;
	}
	
	return 0;
} /* End of SGTL5000PathEnable */

static HWA_DigitalGain SGTL5000PathDisable(unsigned char path)
{
	HWA_SGTL5000_LOG("SGTL5000PathDisable: CTRL path = %d", path);

        if(path >= SGTL5000_PATH_MAX_ID)
        {
                return -1;
        }

        if(SGTL5000SetFunction(&sgtl5000_funcs_off[path]) < 0)
	{
		return -1;
	}

    	return 0;
} /* End of SGTL5000PathDisable */

static HWA_DigitalGain SGTL5000PathVolumeSet(unsigned char path, HWA_AudioVolume volume)
{
	return 0;
} /* End of SGTL5000PathVolumeSet */


static HWA_DigitalGain SGTL5000PathMute(unsigned char path, HWA_AudioMute mute)
{
	return 0;
} /* End of SGTL5000PathMute */


static short SGTL5000GetPathsStatus(char* data, short length)
{
	return 0;
} /* End of SGTL5000GetPathsStatus */


static HWA_AnalogGain SGTL5000GetPathAnalogGain(unsigned char path)
{
	return 0;
} /* End of SGTL5000GetPathAnalogGain */

static HWA_ReturnCode SGTL5000SetPowerMode(HWA_PowerMode mode)
{
    	return 0;
} /* End of SGTL5000SetPowerMode*/

static int SGTL5000SetFunction(SGTL5000_FUNCTION *func)
{
	int err, i, items;
	const char *name = func->function_name;
	const char *value = func->function_value;
	snd_ctl_t *handle;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_info_t *info;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_info_alloca(&info);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, name);
	snd_ctl_elem_info_set_id(info, id);

	if ((err = snd_ctl_open(&handle, sgtl5000_ctl, 0)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000SetFunction: Set register snd_ctl_open error %s", snd_strerror(err));
		return err;
	}

	err = snd_ctl_elem_info(handle, info);
	if (err < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000SetFunction: Control '%s' cannot get element info: %d", name, err);
		snd_ctl_close(handle);
        	return err;
   	}

   	items = snd_ctl_elem_info_get_items(info);

   	for (i = 0; i < items; i++) 
	{
		snd_ctl_elem_info_set_item(info, i);
		err = snd_ctl_elem_info(handle, info);

		if (err < 0) continue;

		if (strcmp(value, snd_ctl_elem_info_get_item_name(info)) == 0)
		{
			err = SGTL5000Set(handle, name, i, -1);
			snd_ctl_close(handle);
			return err;
		}
   	}	

   	HWA_SGTL5000_LOG("SGTL5000SetFunction: Control '%s' has no enumerated value of '%s'", name, value);

	snd_ctl_close(handle);

   	return 0;
}

static int SGTL5000Set(snd_ctl_t *handle, const char *name, unsigned int value, int index)
{
	if (!handle) 
	{
		HWA_SGTL5000_LOG("SGTL5000Set: Control not initialized");
		return -1;
	}

	int i, err, count;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_info_t *info;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_info_alloca(&info);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, name);
	snd_ctl_elem_info_set_id(info, id);

	err = snd_ctl_elem_info(handle, info);
	if (err < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000Set: Control '%s' cannot get element info: %d", name, err);
		return err;;
	}

	count = snd_ctl_elem_info_get_count(info);
	if (index >= count) 
	{
		HWA_SGTL5000_LOG("SGTL5000Set: Control '%s' index is out of range (%d >= %d)", name, index, count);
		return count;
	}

	if (index == -1)
	{
		index = 0; // Range over all of them
	}
	else
	{
		count = index + 1; // Just do the one specified
	}

	snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(info);

	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);

	snd_ctl_elem_info_get_id(info, id);
	snd_ctl_elem_value_set_id(control, id);


	for (i = index; i < count; i++)
	{
		switch (type) 
		{
			case SND_CTL_ELEM_TYPE_BOOLEAN:
				snd_ctl_elem_value_set_boolean(control, i, value);
				break;
			case SND_CTL_ELEM_TYPE_INTEGER:
				snd_ctl_elem_value_set_integer(control, i, value);
				break;
			case SND_CTL_ELEM_TYPE_INTEGER64:
				snd_ctl_elem_value_set_integer64(control, i, value);
				break;
			case SND_CTL_ELEM_TYPE_ENUMERATED:
				snd_ctl_elem_value_set_enumerated(control, i, value);
				break;
			case SND_CTL_ELEM_TYPE_BYTES:
				snd_ctl_elem_value_set_byte(control, i, value);
				break;
			default:
				break;
		}
	}

	err = snd_ctl_elem_write(handle, control);
	HWA_SGTL5000_LOG("SGTL5000Set: snd_ctl_elem_write err = %d", err);

	return err;
}

static int SGTL5000WriteRegisters(unsigned short sgtl5000RegNumid, unsigned short sgtl5000RegVal)
{
	snd_ctl_t *handle;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_type_t type;

	int err,i;
	unsigned int idx, count;
	int numid = 0;
	int value = 0;

	if(sgtl5000RegNumid >= CHIP_MAX_NUMID)
	{
		HWA_SGTL5000_LOG("SGTL5000WriteRegisters: write an invalid register!!");
		return -1;
	}

	numid = (int)sgtl5000RegNumid + NUMID_OFFSET;
	value = (int)sgtl5000RegVal;

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_numid(id, numid);

	if ((err = snd_ctl_open(&handle, sgtl5000_ctl, 0)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000WriteRegisters: snd_ctl_open error = %d", err);
		return -1;
	}

	snd_ctl_elem_info_set_id(info, id);

	if ((err = snd_ctl_elem_info(handle, info)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000WriteRegisters: snd_ctl_elem_info error = %d", err);
		goto failed;
	}

	snd_ctl_elem_info_get_id(info, id);
	type = snd_ctl_elem_info_get_type(info);
	count = snd_ctl_elem_info_get_count(info);
	snd_ctl_elem_value_set_id(control, id);
	
	for (idx = 0; idx < count && idx < 128 ; idx++)
	{
		switch (type) 
		{
			case SND_CTL_ELEM_TYPE_INTEGER:
				if((value >= snd_ctl_elem_info_get_min(info)) && (value <= snd_ctl_elem_info_get_max(info)))			  
				{
					snd_ctl_elem_value_set_integer(control, idx, value);
				}
				break;
			case SND_CTL_ELEM_TYPE_ENUMERATED:
				if ( ((unsigned int)value <= (snd_ctl_elem_info_get_items(info) - 1)) && (value >=0))
				{
					snd_ctl_elem_value_set_enumerated(control, idx, value);
				}
				break;
			default:
				break;
		}
	}

	if ((err = snd_ctl_elem_write(handle, control)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000WriteRegisters: snd_ctl_elem_write error = %d", err);
		goto failed;
	}

	snd_ctl_close(handle);

	HWA_SGTL5000_LOG("SGTL5000WriteRegisters:write register = %d, val = 0x%x successful!", numid, value);

	return 0;

failed:
	snd_ctl_close(handle);
	HWA_SGTL5000_LOG("SGTL5000WriteRegisters error!!!!");

	return -1;
}

static int SGTL5000ReadRegisters(unsigned short sgtl5000RegNumid, unsigned short *sgtl5000RegVal)
{
	snd_hctl_t *hctl;
	snd_ctl_t *handle;
	snd_hctl_elem_t *elem;

	snd_ctl_elem_info_t *info;
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_type_t type;

	int err;
	unsigned int count,idx;
	int numid = 0;
	int value = 0;

	if(sgtl5000RegNumid >= CHIP_MAX_NUMID)
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: Read an invalid register!!");
		return -1;
	}

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_id_alloca(&id);

	numid = (int)sgtl5000RegNumid + NUMID_OFFSET;

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_numid(id, numid);
	
	if ((err = snd_ctl_open(&handle, sgtl5000_ctl, 0)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_ctl_open error = %d", err);
		return err;
	}

	snd_ctl_elem_info_set_id(info, id);

	if ((err = snd_ctl_elem_info(handle, info)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_ctl_elem_info error = %d", err);
		goto failed;
	}

	snd_ctl_elem_info_get_id(info, id);
	type = snd_ctl_elem_info_get_type(info);
	count = snd_ctl_elem_info_get_count(info);
	snd_ctl_elem_value_set_id(control, id);

	snd_ctl_close(handle);

	if ((err = snd_hctl_open(&hctl, sgtl5000_ctl, 0)) < 0)
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_hctl_open error = %d", err);
		return err;
	}

	if ((err = snd_hctl_load(hctl)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_hctl_load error = %d", err);
		goto failed1;
	}

	elem = snd_hctl_find_elem(hctl, id);

	if(!elem)
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_hctl_find_elem elem = %d", elem);
		goto failed1;
	}

	snd_ctl_elem_info_alloca(&info);

	if ((err = snd_hctl_elem_info(elem, info)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_hctl_elem_info error = %d", err);
		goto failed1;
	}

	count = snd_ctl_elem_info_get_count(info);
	type = snd_ctl_elem_info_get_type(info);

	if ((err = snd_hctl_elem_read(elem, control)) < 0) 
	{
		HWA_SGTL5000_LOG("SGTL5000ReadRegisters: snd_hctl_elem_read error = %d", err);
		goto failed1;
	}

	for (idx = 0; idx < count ; idx++)
	{
		switch (type) 
		{
			case SND_CTL_ELEM_TYPE_INTEGER:
				value = snd_ctl_elem_value_get_integer(control, idx);
				break;
			case SND_CTL_ELEM_TYPE_ENUMERATED:
				value = snd_ctl_elem_value_get_enumerated(control, idx); 
				break;
			default:
				break;
		}
	}

	*sgtl5000RegVal = (unsigned short)value;

	HWA_SGTL5000_LOG("SGTL5000ReadRegisters:read register = %d, val = 0x%x successful!", numid, value);

	snd_hctl_close(hctl);

	return 0;

failed:
	snd_ctl_close(handle);
	HWA_SGTL5000_LOG("SGTL5000ReadRegisters error!!!!");
	return -1;

failed1:
	snd_hctl_close(hctl);
	HWA_SGTL5000_LOG("SGTL5000ReadRegisters error 1!!!!");
	return -1;
}

static void SGTL5000ReadCalibrationFile(void)
{
	FILE *sgtl5000_config_fd = NULL;

    	sgtl5000_config_fd = fopen(SGTL5000_CONFIG_FILENAME, "rb");
    	if(sgtl5000_config_fd == NULL)
    	{
        	HWA_SGTL5000_LOG("SGTL5000ReadCalibrationFile: Faild to open %s,will read default params", SGTL5000_CONFIG_FILENAME);
		SGTL5000ReadDefaultConfigParameters();

		return;
    	}
	else
	{
        	HWA_SGTL5000_LOG("SGTL5000ReadCalibrationFile: open %s and read params", SGTL5000_CONFIG_FILENAME);
		SGTL5000ReadConfigParameters(sgtl5000_config_fd);
	}

    	fclose(sgtl5000_config_fd);

	return;
}

static void SGTL5000ReadDefaultConfigParameters(void)
{
	unsigned short register_id = 0;

	for(register_id = 0; default_register_description[register_id].register_id <= CHIP_MAX_NUMID; register_id++)
	{
		if (default_register_description[register_id].register_val > 0)
		{
			SGTL5000WriteRegisters((unsigned short)default_register_description[register_id].register_id, (unsigned short)default_register_description[register_id].register_val);
		}
	}

	return;
}

static void SGTL5000ReadConfigParameters(FILE *sgtl5000_config_fd)
{
	unsigned short register_id = 0;
	unsigned short sgtl5000_register_id = 0;
	unsigned short sgtl5000_register_val = 0;
	signed char buffer[MAX_LENGTH];

	while(register_id <= CHIP_MAX_NUMID)
	{
		calibration_register_description[register_id].register_id = (SGTL5000_REGISTER)register_id;
		calibration_register_description[register_id].register_val = -1;
		register_id++;
	}

    	while((fgets((char *)buffer, MAX_LENGTH, sgtl5000_config_fd)) != NULL)
    	{
        	if ((sscanf((char *)buffer, "numid=%hu,val=0x%hx", &sgtl5000_register_id, &sgtl5000_register_val)) != 2) 
        	{
            		continue;
        	}

        	if ((SGTL5000_REGISTER)sgtl5000_register_id < CHIP_MAX_NUMID)
		{
			calibration_register_description[sgtl5000_register_id].register_id = sgtl5000_register_id;
			calibration_register_description[sgtl5000_register_id].register_val = (short)sgtl5000_register_val;
			SGTL5000WriteRegisters(sgtl5000_register_id, sgtl5000_register_val);
		}
		else
		{
			break;
		}
    	}

	return;
}
