/*
*	Module:	HMSHardIronCal.c
*	Name:	Bruce R. Graham
*	Date:	11/2/2009
*
*	Description: This library provides a simple algorithm
*	to calculate Hard Iron correction offsets using the 
*	average of the maximum and minimum values in the provided
*	data set.
*
*	Version: 1.0
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#include <tgmath.h>
#include "HMSHardIronCal.h"

// Local Definitions

// Local Variables
static int Xmax 		= 0;
static int Ymax 		= 0;
static int Zmax 		= 0;
static int Xmin 		= 0;
static int Ymin 		= 0;
static int Zmin 		= 0;
static int magOffsetX 	= 0;
static int magOffsetY 	= 0;
static int magOffsetZ 	= 0;
static int calThreshold = 0;
static char bCalibrated = 0;

// Local Function Prototypes
static void Zero();


/********************************************************************
*	FUNCTION DEFINITION SECTION
********************************************************************/

/*
*	This function returns the magnetic Hard Iron integer offsets if 
*	they exist. Otherwise, zeros are returned.
*
*	Arguments:
*	"pOffsets" - A pointer to an array of 3 integers.	(Output)
*/
void GetMagOffsets(int* pOffsets)
{
	if (pOffsets)
	{
		if (1 == bCalibrated)
		{
			pOffsets[0] = magOffsetX;
			pOffsets[1] = magOffsetY;
			pOffsets[2] = magOffsetZ;			
		}
		else
		{
			pOffsets[0] = 0;
			pOffsets[1] = 0;
			pOffsets[2] = 0;						
		}
	}
}

/*
*	This function is used to generate the Hard Iron magnetic offsets
*	once the magnetic data has been acquired by the one of the collect
*	data functions.
*
*	Arguments:
*	"pOffsets" - A pointer to an array of 3 integers.	(Output)
*
*	Return:
*		0:	Failed
*		1:	Sucessful
*		2:	Sucessful, but no integer array pointer was provided.
*/
int Calibrate(int* pOffsets)
{		
		// Calculate the Hard Iron offset values
	if (abs(Xmax - Xmin) > calThreshold)
		magOffsetX = ( Xmax + Xmin) / 2;
	if (abs(Ymax - Ymin) > calThreshold)
		magOffsetY = ( Ymax + Ymin) / 2;
	if (abs(Zmax - Zmin) > calThreshold)
		magOffsetZ = ( Zmax + Zmin) / 2;
	
	// Set the Calibrated Flag
	bCalibrated = 1;
	
	// Set the output variables to the offset values
	if (pOffsets)
	{
		pOffsets[0] = magOffsetX;
		pOffsets[1] = magOffsetY;
		pOffsets[2] = magOffsetZ;
	}

	if (!pOffsets)
		return 2;
	return 1;
}

/*
*	Used to set the calibration max/min delta threshold.
*	Generally, this either zero or a positive value.
*/
void SetCalibrationThreshold(int value)
{
	calThreshold = value;
	if (calThreshold < 0)
		calThreshold = -calThreshold;
}

int GetCalibrationThreshold()
{
	return calThreshold;
}

/*
*	Used to pass previouly collected arrays of 3D magnetic data points
*	to the Hard Iron calibration algorithm.
*
*	Arguments:
*	"magX[]" 	- Magnetic data array in units of +/- Counts.	(Input)
*	"magY[]" 	- Magnetic data array in units of +/- Counts.	(Input)
*	"magZ[]" 	- Magnetic data array in units of +/- Counts.	(Input)
*	"dataCount" - Number of data samples collected.				(Input)
*
*	Return:
*		1:	Sucessful
*		0:	Failed - Data count was not recognized.	
*/
int CollectDataArray(int magX[], int magY[], int magZ[], int dataCount)
{
	int length = 0;
	int ix;
	
	// Check for valid data sample count
	if (dataCount > 0)
		length = dataCount;
	else
		return 0;
		
	// Now iterate over the length of the data arrays and
	// pass the magnetic data into the Hard Iron algorithm.
	for (ix = 0; ix < length; ix++)
	{
		CollectDataItem(magX[ix],magY[ix],magZ[ix]);	
	}
	return 1;
}

/*
*	Used to collect 3D magnetic data points one point at a time for
*	use by the Hard Iron calibration algorrithm.
*
*	Arguments:
*	"magX" - Magnetic data in units of +/- Counts.	(Input)
*	"magY" - Magnetic data in units of +/- Counts.	(Input)
*	"magZ" - Magnetic data in units of +/- Counts.	(Input)	
*/
void CollectDataItem(int magX, int magY, int magZ)
{
	if (magX > Xmax)
		Xmax = magX;
	if (magX < Xmin)
		Xmin = magX;
		
	if (magY > Ymax)
		Ymax = magY;
	if (magY < Ymin)
		Ymin = magY;
		
	if (magZ > Zmax)
		Zmax = magZ;
	if (magZ < Zmin)
		Zmin = magZ;
}

/*
*	Initializes the magnetic calibration algorithm to a magnetic starting
*	state using regular/normal magnetic sensed data.
*
*	Arguments:
*	"magX" - Magnetic data in units of +/- Counts.	(Input)
*	"magY" - Magnetic data in units of +/- Counts.	(Input)
*	"magZ" - Magnetic data in units of +/- Counts.	(Input)
*/
void InitialMagneticData(int magX, int magY, int magZ)
{
	// Reset the state variables 
	Zero();
	
	// Now initialize the max and min variables to start values
	Xmax = magX;
	Ymax = magY;
	Zmax = magZ;
	Xmin = magX;
	Ymin = magY;
	Zmin = magZ;
}

/*
*	This function is used to zero most of the module state variables
*/
void Zero()
{
	Xmax 		= 0;
	Ymax 		= 0;
	Zmax 		= 0;
	Xmin 		= 0;
	Ymin 		= 0;
	Zmin 		= 0;
	magOffsetX 	= 0;
	magOffsetY 	= 0;
	magOffsetZ 	= 0;
	bCalibrated = 0;
}




