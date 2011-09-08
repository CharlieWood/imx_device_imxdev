/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: Audio codec hardware adapter layer.
*
* Filename: hwatypes.h
*
* Authors: Nathan Ren
*
* Description: Header file for all HWA types.
*
* Last Updated:
*
* Notes:
******************************************************************************/

#ifndef HWA_TYPES_H
#define HWA_TYPES_H

#ifdef __cplusplus
extern "C"{
#endif

typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned short UINT16;
typedef short INT16;
typedef unsigned char UINT8;
typedef char INT8;

typedef char BOOL;
#define FALSE (char)0
#define TRUE  (char)1

typedef enum
{
    HWA_RC_OK = 0,
    HWA_RC_ERROR,
    HWA_RC_INIT_FAILED,
    HWA_RC_RESET_FAILED,
    HWA_RC_DEVICE_ALREADY_ENABLED,
    HWA_RC_DEVICE_ALREADY_DISABLED,
    HWA_RC_NO_MUTE_CHANGE_NEEDED,
    HWA_RC_INVALID_VOLUME_CHANGE,
    HWA_RC_DEVICE_ROUTE_NOT_FOUND,
    HWA_RC_INVALID_COMPONENT_INDEX,

    HWA_RC_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_ReturnCode;

typedef enum
{
    HWA_MUTE_OFF = 0,
    HWA_MUTE_ON = 1,

    HWA_AUDIO_MUTE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_AudioMute;

typedef enum
 {
    HWA_EARSPEAKER = 0, //earspeaker
    HWA_INT_MIC,    // internel mic
    HWA_LOUDSPEAKER, //loudspeaker
    HWA_LOUD_MIC, //internal loud mic
    HWA_HP_SPEAKER, //headphone speaker
    HWA_HP_LEFT_SPEAKER, //headphone left speaker
    HWA_HP_RIGHT_SPEAKER, //headphone right speaker
    HWA_HP_MIC, //headphone mic
    HWA_BLUETOOTH_SPEAKER, // Bluetooth speaker
    HWA_BLUETOOTH_MIC, // Bluetooth mic
    HWA_AUX1, // AUX1 input
    HWA_AUX2, // AUX2 input
    HWA_AUX3, // AUX3 input

    HWA_NOT_CONNECTED, 
    HWA_NUM_OF_AUDIO_DEVICES = HWA_NOT_CONNECTED, 

    HWA_AUDIO_DEVICE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum enforement
}
HWA_AudioDevice;

typedef enum
{
    HWA_PCM = 0, // digital PCM, mono output and input
    HWA_I2S, // digital I2S, stereo output
    HWA_AUX_FM, // analog line in, stereo FM signal input
    HWA_AUX_HW_MIDI, // MIDI input

    /* Must be at the end */
    HWA_NO_ROUTE,
    HWA_NUM_OF_AUDIO_ROUTES = HWA_NO_ROUTE,

    HWA_AUDIO_ROUTE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum enforcement
} HWA_AudioRoute;

typedef enum
{
    HWA_ROUTE_NOT_SUPPORTED = 0,
    HWA_ROUTE_SUPPORTED = 1,

    HWA_ROUTE_SUPPORTED_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_RouteSupported;

typedef enum
{
    HWA_DEVICE_DISABLE = 0,
    HWA_DEVICE_ENABLE,
    
    HWA_DEVICE_ENABLE_DISABLE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
}HWA_DeviceEnableDisable;

typedef enum
{
    HWA_SPEAKER_PHONE = 0,
    HWA_HANDSET,
    HWA_HEADPHONE,
    HWA_AEC_STATUS,
    HWA_ATCHANNEL_STATUS,
    HWA_BT_HEADSET,
    HWA_CAR_KIT,
    HWA_BT_SPEAKER_PHONE,
    HWA_BT_CAR_KIT,
    HWA_BT_HANDSET,

    HWA_APPLIANCE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_Appliance;

typedef enum
{
    HWA_POWER_OFF = 0,
    HWA_POWER_ASLEEP,
    HWA_POWER_AWAKE,

    HWA_POWER_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_PowerMode;

typedef enum
{
    HWA_PCM_I2S_PLAYBACK = 0,
    HWA_PCM_PCM_PLAYBACK,
    HWA_PCM_PCM_CAPTURE,
    HWA_PCM_AC97_HIFI_PLAYBACK,
    HWA_PCM_AC97_HIFI_CAPTURE,
    HWA_PCM_AC97_AUX_PLAYBACK,

    HWA_PCM_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_PCMDevice;

typedef enum
{
    HWA_PCM_FORMAT_S16_LE = 0,

    HWA_PCM_FORMAT_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement    
}HWA_PCMFormat ;

typedef unsigned char HWA_AudioVolume;  /* (0 - 100)% */
typedef signed char   HWA_DigitalGain; /* dB */
typedef signed char   HWA_AnalogGain; /* dB */

typedef    void            (*HWAComponentInit_t)  (unsigned char reinit);
typedef    HWA_DigitalGain (*HWAEnablePath_t)     (unsigned char path, HWA_AudioVolume volume);
typedef    HWA_DigitalGain (*HWADisablePath_t)    (unsigned char path);
typedef    HWA_DigitalGain (*HWAVolumeSetPath_t)  (unsigned char path, HWA_AudioVolume volume);
typedef    HWA_DigitalGain (*HWAMutePath_t)       (unsigned char path, HWA_AudioMute mute);
typedef    short (*HWAGetPathsStatus_t) (char* data, short length);
typedef    HWA_AnalogGain (*HWAGetPathAnalogGain_t)    (unsigned char path);
typedef    HWA_ReturnCode (*HWASetPowerMode_t)     (HWA_PowerMode mode);


typedef struct
{
    HWAComponentInit_t                HWAComponentInit;
    HWAEnablePath_t                    HWAEnablePath;
    HWADisablePath_t                   HWADisablePath;
    HWAVolumeSetPath_t              HWAVolumeSetPath;
    HWAMutePath_t                       HWAMutePath;
    HWAGetPathsStatus_t             HWAGetPathsStatus;
    HWAGetPathAnalogGain_t	     HWAGetPathAnalogGain;
    HWASetPowerMode_t              HWASetPowerMode;
} HWA_ComponentHandle;

typedef struct
{
    HWA_AudioDevice            device;
    HWA_AudioRoute             route;
    HWA_Component              component;
    unsigned long              path;
    HWA_Appliance              appliance;
} HWA_DeviceRoute;

typedef struct
{
    HWA_DeviceRoute         deviceRoute;
    HWA_AudioMute           deviceMute;
    HWA_DeviceEnableDisable deviceEnableDisable;
    HWA_DigitalGain         deviceDigitalGain;
    HWA_AnalogGain          deviceAnalogGain;
    HWA_AudioVolume         deviceVolume;
}HWA_DeviceRouteConfig;
/*----------- Extern definition ----------------------------------------------*/

/*----------- Global variable declarations -----------------------------------*/

/*----------- Global function prototypes -------------------------------------*/

/*----------- Global macro definitions ---------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif  /* HWA_TYPES_H */


