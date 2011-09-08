
#ifndef _HMSHEADING_H_
#define _HMSHEADING_H_

#define DegToRad .0174533
#define RadToDeg 57.29578

/*
*	Error States - returned by the Heading() function.
*/
#define HEADING_ERROR_UNKNOWN			-1
#define HEADING_ERROR_ARGUMENT_NULL		-2
#define HEADING_ERROR_OUT_OF_RANGE		-3
#define HEADING_ERROR_DIVIDE_BY_ZERO	-4
#define HEADING_ERROR_NOT_FINITE_NUMBER	-5

// ******************Function Prototypes*************************************

/*
*	This function calculates magnetic heading based on a combination of magnetic
*	and acceleration data provided by the User. Output range is from 0 to 360 Degrees.
*	Magnetic data is in units of Counts centered around 0. i.e. from -N to N -1.
*
*	Example: 12-bit ADC. Range: 0 to 4095 needs to be translated to -2048 to 2047.
*	Acceleration data is in units of G's and in the range of -1 to 1.
*
*	Error codes that are less than zero are returned if an error has been detected 
*	during the calculation of the heading.
*
*	"magForward" - Forward magnetic measurement in units of +/- Counts.
*	"magLeft"	 - Left magnetic measurement in units of +/- Counts.
*	"magUp"		 - Up magnetic measurement in units of +/- Counts.
*	"accForward" - Forward acceleration measurement in units of G's. Range: -1 to 1.
*	"accLeft"	 - Left acceleration measurement in units of G's. Range: -1 to 1.
*
*	Return:	0 to 360 Degrees. Values less than 0 are error messages.
*/
double Heading(int magForward,int magLeft,int magUp,float accForward,float accLeft);

#endif