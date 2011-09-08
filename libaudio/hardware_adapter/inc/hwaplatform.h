/* Copyright © 2010, Letou Tech Co., Ltd. All rights reserved.
   Letou Tech Co., Ltd. Confidential Proprietary
   Contains confidential proprietary information of Letou Tech Co., Ltd.
   Reverse engineering is prohibited.
   The copyright notice does not imply publication. */
/******************************************************************************
* Title: Audio codec hardware adapter layer.
*
* Filename: hwaplatform.h
*
* Authors: Nathan Ren
*
* Description: HWA platform components header
*
* Last Updated:
*
* Notes:
******************************************************************************/
#ifndef HWA_PLATFORM_H
#define HWA_PLATFORM_H
#ifdef __cplusplus
extern "C"{
#endif

/*----------- Global type definitions ----------------------------------------*/


/*----------- Local include files --------------------------------------------*/
#include "hwaplatformcomp.h"
#include "hwatypes.h"

/***   Platform Components list ****/
/* Add new platform components here */
#include "audiogpoapi.h"
#include "audiosgtl5000api.h"

//Null component
#include "audionullapi.h"

/*----------- Global defines -------------------------------------------------*/


/*----------- Global macro definitions ---------------------------------------*/


/*----------- Extern definition ----------------------------------------------*/


/*----------- Global variable declarations -----------------------------------*/


/*----------- Global constant definitions ------------------------------------*/


/*----------- Global function prototypes -------------------------------------*/

#ifdef __cplusplus
}
#endif
#endif  /* HWA_PLATFORM_H */


