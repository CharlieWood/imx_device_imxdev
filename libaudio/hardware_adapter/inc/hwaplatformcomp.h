/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: Audio codec hardware adapter layer
*
* Filename: hwaplatformcomp.h
*
* Authors: Nathan Ren
*
* Description: HWA platform components header
*
* Last Updated:
*
* Notes:
******************************************************************************/
#ifndef HWA_PLATFORM_COMPONENTS_H
#define HWA_PLATFORM_COMPONENTS_H

#ifdef __cplusplus
extern "C"{
#endif

/*----------- Local include files --------------------------------------------*/

/*----------- Global defines -------------------------------------------------*/

/*----------- Global macro definitions ---------------------------------------*/

/*----------- Global constant definitions ------------------------------------*/

/*----------- Global type definitions ----------------------------------------*/

typedef enum
{
    GPO_COMPONENT,
    SGTL5000_COMPONENT,	
    NULL_COMPONENT,          //must be last one
    HWA_COMPONENTS_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} HWA_Component;

/*----------- Extern definition ----------------------------------------------*/

/*----------- Global variable declarations -----------------------------------*/


/*----------- Global function prototypes -------------------------------------*/


#ifdef __cplusplus
}
#endif
#endif  /* _HWA_PLATFORM_COMPONENTS_H_ */


