/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: Audio codec hardware adapter layer.
*
* Filename: hwa.h
*
* Authors: Nathan Ren
*
* Description: Header file for HWA.
*
* Last Updated:
*
* Notes:
******************************************************************************/

#ifndef HWA_H
#define HWA_H 

#ifdef __cplusplus
extern "C"{
#endif

/*----------- Local include files --------------------------------------------*/
#include "hwaplatformcomp.h"
#include "hwatypes.h"

/*----------- Global macro definitions ---------------------------------------*/

/*----------- Global type definitions ----------------------------------------*/

/*----------- Global structure definitions ----------------------------------------*/

/*----------- Extern definition ----------------------------------------------*/

/*----------- Global variable declarations -----------------------------------*/

/*----------- Global constant definitions ------------------------------------*/
#define HWA_INDEX_MODULE_OFF 0x0FFL

/*----------- Global API function definition ---------------------------------*/
HWA_ReturnCode HWA_Init(void);
HWA_ReturnCode HWA_Reset(void);
HWA_ReturnCode HWA_Deinit(void);
HWA_ReturnCode HWA_AudioDeviceEnable(HWA_AudioDevice device, HWA_AudioRoute route, HWA_AudioVolume volume);
HWA_ReturnCode HWA_AudioDeviceDisable(HWA_AudioDevice device, HWA_AudioRoute route);
HWA_RouteSupported HWA_AudioRouteSupported(HWA_AudioDevice device, HWA_AudioRoute route);
HWA_ReturnCode HWA_AudioDeviceVolumeSet(HWA_AudioDevice device, HWA_AudioRoute route, HWA_AudioVolume volume);
HWA_ReturnCode HWA_AudioDeviceMute(HWA_AudioDevice device, HWA_AudioRoute route, HWA_AudioMute mute);
HWA_ReturnCode HWA_SetPowerMode(HWA_Component component, HWA_PowerMode mode);

#ifdef __cplusplus
}
#endif
#endif  /* _HWA_H_ */


