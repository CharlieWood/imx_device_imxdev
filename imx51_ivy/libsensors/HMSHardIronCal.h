
#ifndef _HMSHARDIONCAL_H_
#define _HMSHARDIONCAL_H_

/*
*	Initializes the magnetic calibration algorithm to a magnetic starting
*	state using regular/normal magnetic sensed data.
*
*	Arguments:
*	"magX" - Magnetic data in units of +/- Counts.	(Input)
*	"magY" - Magnetic data in units of +/- Counts.	(Input)
*	"magZ" - Magnetic data in units of +/- Counts.	(Input)
*/
void InitialMagneticData(int magX, int magY, int magZ);

/*
*	Used to collect 3D magnetic data points one point at a time for
*	use by the Hard Iron calibration algorrithm.
*
*	Arguments:
*	"magX" - Magnetic data in units of +/- Counts.	(Input)
*	"magY" - Magnetic data in units of +/- Counts.	(Input)
*	"magZ" - Magnetic data in units of +/- Counts.	(Input)	
*/
void CollectDataItem(int magX, int magY, int magZ);

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
int CollectDataArray(int magX[], int magY[], int magZ[], int dataCount);

/*
*	Used to set the calibration max/min delta threshold.
*	Generally, this either zero or a positive value.
*/
void SetCalibrationThreshold(int value);
int GetCalibrationThreshold();

/*
*	This function is used to generate the Hard Iron magnetic offsets
*	once the magnetic data has been acquired by the one of the collect
*	data functions.
*
*	Arguments:
*	"pOffsets" - A pointer to an array of 3 integers representing the magnetic offsets.	(Output)
*				pOffsets[0] = X, pOffsets[1] = Y, and pOffsets[2] = Z.
*
*	Return:
*		0:	Failed
*		1:	Sucessful
*		2:	Sucessful, but no integer array pointer was provided.
*/
int Calibrate(int* pOffsets);

/*
*	This function returns the magnetic Hard Iron integer offsets if 
*	they exist. Otherwise, zeros are returned.
*
*	Arguments:
*	"pOffsets" - A pointer to an array of 3 integers representing the magnetic offsets.	(Output)
*				pOffsets[0] = X, pOffsets[1] = Y, and pOffsets[2] = Z.
*/
void GetMagOffsets(int* pOffsets);

#endif
