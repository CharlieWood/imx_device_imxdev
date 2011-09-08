/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name			: hmc5883.h
* Original File Name	: lsm303dlh.h
* Original Authors	: MH - C&I BU - Application Team
*				: Carmine Iascone (carmine.iascone@st.com)
*				: Matteo Dameno (matteo.dameno@st.com)
* Original Version	: V 0.3
* Original File Date	: 24/02/2010
*
********************************************************************************
*
* Original STMicroelectronics Notice:
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
*******************************************************************************/

/******************** PORTIONS (C) COPYRIGHT 2010 Honeywell ********************
*
* Modified Author		: Ron Fang
* Modified Version	: V 0.1
* Modified Date		: 05/10/2010
*
* Honeywell Notice:
* This program contains open source software that is covered by the GNU General
* Public License version 2 as published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, HONEYWELL SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT
* OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH HONEYWELL PARTS.
*
********************************************************************************
*
* Modified Items by Honeywell:
* 1. Replacing all references to "lsm303dlh_mag" by "hmc5883" including:
*    filename, variable names, function names, case names, comments, and texts.
* 2. Modified magnetometer "full scale range" values to match HMC5883 datasheet.
*    Added definition for range 0x00.
* 3. Removed all references and definitions related to "Accelerometer Sensor".
* 4. Added difinition for "IDLE_MODE".
* 5. Corrected difinition of output rate for 7.5Hz from 0x09 to 0x0C.
*
*******************************************************************************/


#ifndef __HMC5883_H__
#define __HMC5883_H__

#include <linux/ioctl.h>  /* For IOCTL macros */

#define HMC5883_IOCTL_BASE 'm'
/* The following define the IOCTL command values via the ioctl macros */
#define HMC5883_SET_RANGE		_IOW(HMC5883_IOCTL_BASE, 1, int)
#define HMC5883_SET_MODE		_IOW(HMC5883_IOCTL_BASE, 2, int)
#define HMC5883_SET_BANDWIDTH		_IOW(HMC5883_IOCTL_BASE, 3, int)
#define HMC5883_READ_MAG_XYZ		_IOW(HMC5883_IOCTL_BASE, 4, int)
#define ORIENTATION_GET_STATUS		_IOW(HMC5883_IOCTL_BASE, 5, int)
#define ORIENTATION_SET_STATUS		_IOW(HMC5883_IOCTL_BASE, 6, int)


/************************************************/
/* 	Magnetometer section defines	 	*/
/************************************************/

/* Magnetometer Sensor Full Scale */
#define HMC5883_0_9G		0x00
#define HMC5883_1_2G		0x20
#define HMC5883_1_9G		0x40
#define HMC5883_2_5G		0x60
#define HMC5883_4_0G		0x80
#define HMC5883_4_6G		0xA0
#define HMC5883_5_5G		0xC0
#define HMC5883_7_9G		0xE0

/* Magnetic Sensor Operating Mode */
#define HMC5883_NORMAL_MODE	0x00
#define HMC5883_POS_BIAS	0x01
#define HMC5883_NEG_BIAS	0x02
#define HMC5883_CC_MODE		0x00
#define HMC5883_SC_MODE		0x01
#define HMC5843_IDLE_MODE	0x02
#define HMC5883_SLEEP_MODE	0x03

/* Magnetometer output data rate  */
#define HMC5883_ODR_75		0x00	/* 0.75Hz output data rate */
#define HMC5883_ODR1_5		0x04	/* 1.5Hz output data rate */
#define HMC5883_ODR3_0		0x08	/* 3Hz output data rate */
#define HMC5883_ODR7_5		0x0C	/* 7.5Hz output data rate */
#define HMC5883_ODR15		0x10	/* 15Hz output data rate */
#define HMC5883_ODR30		0x14	/* 30Hz output data rate */
#define HMC5883_ODR75		0x18	/* 75Hz output data rate */

#ifdef __KERNEL__

struct hmc5883_platform_data {

	u8 h_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);

};
#endif /* __KERNEL__ */

#endif  /* __HMC5883_H__ */
