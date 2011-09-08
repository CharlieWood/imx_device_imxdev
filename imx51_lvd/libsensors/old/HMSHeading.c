/*
*	Module:	HMSHeading.c
*	Name:	Bruce R. Graham
*	Date:	11/2/2009
*
*	Description: This library provides a simple Heading algorithm
*	for navigation purposes that uses 3D magnetic data and 2D ac-
*	celeration data for tilt compensated output.
*
*	Version: 1.0
*/

#include "HMSHeading.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include <tgmath.h>

#define TwoPI	2 * M_PI

// Local Function Prototypes
static double ATAN2(double Y,double X);
static double ASIN(double X);
static int AccelRangeCheck(double value);

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
double Heading(int magForward,int magLeft,int magUp,float accForward,float accLeft)
{
	double S_a_L,C_a_L,HC1,HC2;
	double angle_F,angle_L;
	double C_a_F,S_a_F;
	double Head = HEADING_ERROR_UNKNOWN;
	
	// Check the acceleration range
	if (!AccelRangeCheck(accForward))
		return HEADING_ERROR_OUT_OF_RANGE;
	if (! AccelRangeCheck(accLeft))
		return HEADING_ERROR_OUT_OF_RANGE;
		
	// Get the angles from the accelerations
	angle_F = ASIN(-accForward);
	angle_L = ASIN(-accLeft);
	
	// Generate Sine and Cosine components
	C_a_F = cos(angle_F);
	S_a_F = sin(angle_F);
	
	C_a_L = cos(angle_L);
	S_a_L = sin(angle_L);
	
	// Generate the tilt compensated magnetic horizontal components
	HC1 = C_a_F * magForward - S_a_F * S_a_L * magLeft - S_a_F * C_a_L * magUp;
	HC2 = -C_a_L * magLeft + S_a_L * magUp;
	
	// Calculate the Heading in units of radians.
	Head = -ATAN2(HC2,HC1);
	
	// Range scale it to 0 to 360 degrees
	if (Head < 0)
		Head += TwoPI;
	Head *= RadToDeg;
	
	return Head;
}

/*
*	This function returns the angle in radians of a triangle.
*	The hypotenuse is forced to be 1.
*	"X" - Opposite Side.
*	Return: Radian angle.
*/
double ASIN(double X)
 {
	 double demon = sqrt(-X * X +1);
	return atan2(X,demon);	 
 }

/*
*	This function returns the angle in radians of a triangle.
*	"Y" - Opposite side.
*	"X" - Adjacent Side.
*	Return: Radian angle.
*/
 double ATAN2(double Y,double X)
 {
	return atan2(Y,X);	 
 }

/*
*	Range check the acceleration to +/- 1 G.
*	"value" - The acceleration in units of gravity (G's).
*	Return: 1/0 or True/False.
*/
int AccelRangeCheck(double value)
{
	if (abs(value) <= 1)
		return 1;
	return 0;
}
