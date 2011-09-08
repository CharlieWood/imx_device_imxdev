/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: Audio codec hardware adapter control layer.
*
* Filename: hwa_control.c
*
* Authors: Nathan Ren
*
* Description: HWA main functions file.
*
* Last Updated:
*
* Notes:
******************************************************************************/

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "hwa.h"
#include "hwaplatform.h"
#include "hwa_config.h"

/*----------- Local macro definitions ----------------------------------------*/
#define MASK8(a)	((unsigned char)((a) & 0x000000FF))

/*----------- Local type definitions -----------------------------------------*/

/*----------- Local variable definitions -------------------------------------*/
static HWA_DeviceRouteConfig* _deviceRouteConfigTable;
static HWA_DeviceRoute*       _deviceRouteTable;
static UINT32                 _deviceRouteTableSize;
const HWA_ComponentHandle*    componentHandles[] =
{
    &HWGpoHandle,// Gpo component
    &HWSGTL5000Handle,// SGTL5000 component
    &HWNullHandle, // NULLHW
};

/*******************************************************************************
* Function: HWABspGetBoardTables
*******************************************************************************
* Description: Get HWA Tables according to BSP board type
*
* Parameters: none
*
* Return value: void
*
* Notes:
*******************************************************************************/
static void HWABspGetBoardTables(void)
{
    _deviceRouteTable = (HWA_DeviceRoute *) deviceTable_Borad;
    _deviceRouteTableSize = sizeof(deviceTable_Borad)/sizeof(HWA_DeviceRoute);
}

/*******************************************************************************
* Function: HWA_Init
*******************************************************************************
* Description: 
*
* Parameters: none
*
* Return value: void
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_Init(void)
{
    HWA_DeviceRouteConfig *pDeviceRouteConfig;
    HWA_DeviceRoute       *pDeviceRoute;
    BOOL                   initLocalComponent[NULL_COMPONENT];

    //get specific board device mapping default tables
    HWABspGetBoardTables();

    if (_deviceRouteTableSize == 0)
    {
        return HWA_RC_INIT_FAILED;
    }
    
    pDeviceRoute = _deviceRouteTable;
    pDeviceRouteConfig = _deviceRouteConfigTable = (HWA_DeviceRouteConfig*)malloc(_deviceRouteTableSize*sizeof(HWA_DeviceRouteConfig));

    while (pDeviceRoute->device != HWA_NOT_CONNECTED)
    {
        memcpy(pDeviceRouteConfig, pDeviceRoute, sizeof(HWA_DeviceRoute));
        pDeviceRouteConfig->deviceMute          = HWA_MUTE_OFF;
        pDeviceRouteConfig->deviceEnableDisable = HWA_DEVICE_DISABLE;
        pDeviceRouteConfig->deviceDigitalGain   = 0;
        pDeviceRouteConfig->deviceAnalogGain    = 0;
        pDeviceRouteConfig->deviceVolume        = 0;
        pDeviceRouteConfig++;
        pDeviceRoute++;
    }
    memcpy(pDeviceRouteConfig, pDeviceRoute, sizeof(HWA_DeviceRoute)); //For HWA_NOT_CONNECT

    /* Reset the component init flag array */
    memset(initLocalComponent, FALSE, (NULL_COMPONENT * sizeof(BOOL)));

    pDeviceRouteConfig = _deviceRouteConfigTable;
    while ( (pDeviceRouteConfig->deviceRoute.component != NULL_COMPONENT) )
    {
        if(initLocalComponent[pDeviceRouteConfig->deviceRoute.component] == FALSE )
        {
            componentHandles[pDeviceRouteConfig->deviceRoute.component]->HWAComponentInit(FALSE);
            initLocalComponent[pDeviceRouteConfig->deviceRoute.component] = TRUE;
        }
        pDeviceRouteConfig++;
    }
    return HWA_RC_OK;
} /* End of HWAInit */

/*******************************************************************************
* Function: HWA_Reset
*******************************************************************************
* Description: 
*
* Parameters: none
*
* Return value: void
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_Reset(void)
{
    HWA_DeviceRouteConfig *pDeviceRouteConfig;
    HWA_DeviceRoute       *pDeviceRoute;
    BOOL                   initLocalComponent[NULL_COMPONENT];

    if (_deviceRouteTableSize == 0)
    {
        return HWA_RC_RESET_FAILED;
    }
    
    pDeviceRoute = _deviceRouteTable;
    pDeviceRouteConfig = _deviceRouteConfigTable;

    while (pDeviceRoute->device != HWA_NOT_CONNECTED)
    {
        memcpy(pDeviceRouteConfig, pDeviceRoute, sizeof(HWA_DeviceRoute));
        pDeviceRouteConfig->deviceMute          = HWA_MUTE_OFF;
        pDeviceRouteConfig->deviceEnableDisable = HWA_DEVICE_DISABLE;
        pDeviceRouteConfig->deviceDigitalGain   = 0;
        pDeviceRouteConfig->deviceAnalogGain    = 0;
        pDeviceRouteConfig->deviceVolume        = 0;
        pDeviceRouteConfig++;
        pDeviceRoute++;
    }
    memcpy(pDeviceRouteConfig, pDeviceRoute, sizeof(HWA_DeviceRoute));

    /* Reset the component init flag array */
    memset(initLocalComponent, FALSE, (NULL_COMPONENT * sizeof(BOOL)));

    pDeviceRouteConfig = _deviceRouteConfigTable;
    while ( (pDeviceRouteConfig->deviceRoute.component != NULL_COMPONENT) )
    {
        if(initLocalComponent[pDeviceRouteConfig->deviceRoute.component] == FALSE )
        {
            componentHandles[pDeviceRouteConfig->deviceRoute.component]->HWAComponentInit(TRUE);
            initLocalComponent[pDeviceRouteConfig->deviceRoute.component] = TRUE;
        }
        pDeviceRouteConfig++;
    }
    return HWA_RC_OK;
} /* End of HWAInit */

/*******************************************************************************
* Function: HWA_Deinit
*******************************************************************************
* Description: 
*
* Parameters: none
*
* Return value: void
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_Deinit(void)
{
    free(_deviceRouteConfigTable);
    return HWA_RC_OK;
}

/*******************************************************************************
* Function: HWAAudioDeviceEnable
*******************************************************************************
* Description: Enables the device/route pair and sets the volume.
*               Enable the pair only if  it's not enaled already, and set the
*               analog volume.
*
* Parameters: HWA_AudioDevice device
*             HWA_AudioRoute route
*             HWA_AudioVolume volume
*
* Return value: HWA_ReturnCode
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_AudioDeviceEnable(HWA_AudioDevice device,
                                    HWA_AudioRoute route,
                                    HWA_AudioVolume volume)
{
    HWA_DeviceRouteConfig *handle_p;
    HWA_ReturnCode acmReturnCode = HWA_RC_DEVICE_ROUTE_NOT_FOUND;
    HWA_AnalogGain acmCumAnalogGain = 0;

    handle_p = _deviceRouteConfigTable;

    while (handle_p->deviceRoute.device != HWA_NOT_CONNECTED)
    {
        if ( (handle_p->deviceRoute.device == device) && (handle_p->deviceRoute.route == route) )  /* path for this channel */
        {
            if(handle_p->deviceEnableDisable != HWA_DEVICE_ENABLE)
            {
           /* The path is not being used by other devices or other routes.
            * The check must be done AFTER updating the Data Base (audioRoute[device]),
            * that way, we can see that no other routes left on the specific device */
                acmReturnCode = HWA_RC_OK;
                handle_p->deviceVolume = volume;
                handle_p->deviceDigitalGain = componentHandles[handle_p->deviceRoute.component]->HWAEnablePath(MASK8(handle_p->deviceRoute.path), handle_p->deviceVolume);
                acmCumAnalogGain += componentHandles[handle_p->deviceRoute.component]->HWAGetPathAnalogGain(MASK8(handle_p->deviceRoute.path));
                handle_p->deviceAnalogGain = acmCumAnalogGain;

                handle_p->deviceEnableDisable = HWA_DEVICE_ENABLE;
                componentHandles[handle_p->deviceRoute.component]->HWAMutePath(MASK8(handle_p->deviceRoute.path), handle_p->deviceMute);
            }
            else
            {
                /* The specific pair of device/Route to be enabled is already active */

                HWA_AudioDeviceVolumeSet(device, route, volume);  /* Change the volume (if needed) */
                if( acmReturnCode != HWA_RC_OK )
                    acmReturnCode = HWA_RC_DEVICE_ALREADY_ENABLED;

            }
        }
        handle_p++;
    }

    return acmReturnCode;
} /* End of HWAAudioDeviceEnable */

/*******************************************************************************
* Function: HWAAudioDeviceDisable
*******************************************************************************
* Description: Disables the device/route pair only if the pair enabled.
*
* Parameters: HWA_AudioDevice device
*             HWA_AudioRoute route
*
* Return value: HWA_ReturnCode
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_AudioDeviceDisable(HWA_AudioDevice device,
                                     HWA_AudioRoute route)
{
    HWA_DeviceRouteConfig *handle_p, *handlePending_p;
    HWA_ReturnCode acmReturnCode = HWA_RC_DEVICE_ROUTE_NOT_FOUND;

    handle_p = _deviceRouteConfigTable;

    while (handle_p->deviceRoute.device != HWA_NOT_CONNECTED)
    {
        if ( (handle_p->deviceRoute.device == device) && (handle_p->deviceRoute.route == route) )  /* path for this channel */
        {
            if(handle_p->deviceEnableDisable != HWA_DEVICE_DISABLE)
            {
                handle_p->deviceEnableDisable = HWA_DEVICE_DISABLE;
                handle_p->deviceMute          = HWA_MUTE_OFF;  /* Mark it as un-muted */
                acmReturnCode = HWA_RC_OK;
                handlePending_p = _deviceRouteConfigTable;

                /* Check if there is another user of the found path */
                while (handlePending_p->deviceRoute.device != HWA_NOT_CONNECTED)
                {
                    if (handlePending_p->deviceEnableDisable == HWA_DEVICE_ENABLE)
                    {  /* finding another enabled path */
                        if ( (handlePending_p->deviceRoute.path == handle_p->deviceRoute.path) &&
                                (handlePending_p->deviceRoute.component == handle_p->deviceRoute.component) )
                            {  /* the same path is being used by another user */

                                break;
                            }
                    }
                    handlePending_p++;
                }

                if ( handlePending_p->deviceRoute.device == HWA_NOT_CONNECTED )
                {  /* Getting to the end of the table without finding another user of this specific path */
                    componentHandles[handle_p->deviceRoute.component]->HWADisablePath(MASK8(handle_p->deviceRoute.path));
                }
            }
            else  /*device already disable */
            {
                acmReturnCode = HWA_RC_DEVICE_ALREADY_DISABLED;
            }
        }
        handle_p++;
    }

    return acmReturnCode;
} /* End of HWAAudioDeviceDisable */

/*******************************************************************************
* Function: HWAAudioDeviceVolumeSet
*******************************************************************************
* Description: Sets the analog volume of the device.
*
* Parameters: HWAAudioDevice device
*             HWAAudioRoute route
*             HWAAudioVolume volume
*
* Return value: HWA_ReturnCode
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_AudioDeviceVolumeSet(HWA_AudioDevice device,
                                       HWA_AudioRoute route,
                                       HWA_AudioVolume volume)
{
    HWA_DeviceRouteConfig  *handle_p;
    HWA_ReturnCode  acmReturnCode = HWA_RC_DEVICE_ROUTE_NOT_FOUND;
    HWA_AnalogGain  acmCumAnalogGain = 0;

    handle_p = _deviceRouteConfigTable;

    while (handle_p->deviceRoute.device != HWA_NOT_CONNECTED)
    {
        if ( (handle_p->deviceRoute.device == device) && (handle_p->deviceRoute.route == route) )  /* path for this channel */
        {
           if(handle_p->deviceVolume != volume)
           {
                acmReturnCode = HWA_RC_OK;
                handle_p->deviceVolume = volume;

                if(handle_p->deviceEnableDisable == HWA_DEVICE_ENABLE)
                {
                    handle_p->deviceDigitalGain = componentHandles[handle_p->deviceRoute.component]->HWAVolumeSetPath(MASK8(handle_p->deviceRoute.path),  handle_p->deviceVolume);
                    acmCumAnalogGain += componentHandles[handle_p->deviceRoute.component]->HWAGetPathAnalogGain(MASK8(handle_p->deviceRoute.path));
                    handle_p->deviceAnalogGain = acmCumAnalogGain;
                }
            }
            else
            {  /* Volume did not change - just update the return code */
                acmReturnCode = HWA_RC_OK;
            }

        }

        handle_p++;
    }

    return acmReturnCode;
} /* HWAAudioDeviceVolumeSet */

/*******************************************************************************
* Function: HWAAudioDeviceMute
*******************************************************************************
* Description: Analog mutes or un-mutes the device passed.
*               In case of voice call:
*                   un-mute ==> perform it.
*                   mute ==> perform it only if the device is the only active
rform it.
*                   mute ==> perform it only if the device is the only active
*                       non-muted device on the relevant path (in / out).
*
* Parameters: HWA_AudioDevice device
*             HWA_AudioRoute route
*             HWA_AudioMute   mute
*
* Return value: HWA_ReturnCode
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_AudioDeviceMute(HWA_AudioDevice device,
                                  HWA_AudioRoute route,
                                  HWA_AudioMute   mute)
{
    HWA_DeviceRouteConfig   *handle_p;
    HWA_ReturnCode          acmReturnCode = HWA_RC_DEVICE_ROUTE_NOT_FOUND;

    handle_p = _deviceRouteConfigTable;

    /* Mutes all the paths associated with this channel */

    while  (handle_p->deviceRoute.device != HWA_NOT_CONNECTED)
    {
        if  ((handle_p->deviceRoute.device == device) && (handle_p->deviceRoute.route == route))
        {
            if(handle_p->deviceMute != mute)
            {
                acmReturnCode = HWA_RC_OK;
             	if( handle_p->deviceEnableDisable == HWA_DEVICE_ENABLE )
                {
                    componentHandles[handle_p->deviceRoute.component]->HWAMutePath(MASK8(handle_p->deviceRoute.path), mute);
                }
                handle_p->deviceMute = mute;
            }
            else
            {
                if( acmReturnCode != HWA_RC_OK )
                    acmReturnCode = HWA_RC_NO_MUTE_CHANGE_NEEDED;
            }
        }
        handle_p++;
    }

    return acmReturnCode;
} /* End of HWAAudioDeviceMute */

/*******************************************************************************
* Function: HWA_AudioRouteSupported
*******************************************************************************
* Description: Returns HWA_Route_SUPPORTED if the device supports the Audio
*               route passed, HWA_ROUTE_NOT_SUPPORTED otherwise.
*
* Parameters: HWA_AudioDevice device
*             HWA_AudioRoute route
*
* Return value: HWA_RouteSupported
*
* Notes:
*******************************************************************************/
HWA_RouteSupported HWA_AudioRouteSupported(HWA_AudioDevice device, HWA_AudioRoute route)
{
    HWA_DeviceRouteConfig  *handle_p;

    handle_p = _deviceRouteConfigTable;
    while ( (handle_p->deviceRoute.device != HWA_NOT_CONNECTED) &&
            ((handle_p->deviceRoute.device != device) || (handle_p->deviceRoute.route != route)) )
    {
        handle_p++;
    }

    if(handle_p->deviceRoute.device != HWA_NOT_CONNECTED)
        return HWA_ROUTE_SUPPORTED;
    else
        return HWA_ROUTE_NOT_SUPPORTED;
} /* End of HWA_AudioRouteSupported */

/*******************************************************************************
* Function: HWA_SetPowerMode
*******************************************************************************
* Description: 
*
* Parameters: none
*
* Return value: void
*
* Notes:
*******************************************************************************/
HWA_ReturnCode HWA_SetPowerMode(HWA_Component component, HWA_PowerMode mode)
{
    if (component >= NULL_COMPONENT )
        return HWA_RC_INVALID_COMPONENT_INDEX;
    else
        return componentHandles[component]->HWASetPowerMode(mode);
}
