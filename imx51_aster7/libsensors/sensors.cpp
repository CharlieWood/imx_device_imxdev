/*
 * Copyright (C) 2009 Bosch Sensortec GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "Sensor"
#define HMC5883
#ifndef INFTIM
#define INFTIM       -1
#endif


#include <hardware/hardware.h>
#include <hardware/sensors.h>

#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <stdio.h>

#include <cutils/native_handle.h>

#ifdef HMC5883
#include <math.h>
extern "C"{
#include <HMSHardIronCal.h>
#include <HMSHeading.h>
}
#endif

#define NSEC_PER_SEC	1000000000L

//#define DEBUG_SENSOR	1

static inline int64_t timespec_to_ns(const struct timespec *ts)
{
	return ((int64_t) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

/*****************************************************************************/

#define SENSOR_DATA_DEVICE "/dev/bma150"
#ifdef HMC5883
#define MSENSOR_DATA_DEVICE "/dev/hmc5883"
#endif

//#define SENSOR_DATA_DEVICE "/dev/bma120"
//#define SENSOR_DATA_DEVICE "/dev/bmp085"
//#define PRESSURER
#define ACCELEROMETER
//#define BMA120_SENSOR

#define BMA150_SENSOR
// Convert to Gs
// FIXME!!!! The raw data is suppose to be +-2G with 8bit fraction
// FIXME!!!! We are getting 7bit fraction
#define CONVERT                     (GRAVITY_EARTH / 256)
#define CONVERT_X                   -(CONVERT)
#define CONVERT_Y                   -(CONVERT)
#define CONVERT_Z                   (CONVERT)


#define S_HANDLE_ACCELEROMETER		(1<<SENSOR_TYPE_ACCELEROMETER)
#define S_HANDLE_PRESSURER 			(1<<SENSOR_TYPE_ACCELEROMETER)
#ifdef HMC5883
#define S_HANDLE_MAGNETIC_FIELD		(1<<SENSOR_TYPE_MAGNETIC_FIELD)
#endif


static int s_timeout = 400;

static int s_device_open(const struct hw_module_t* module,
                         const char* name,
                         struct hw_device_t** device);

static int s_get_sensors_list (struct sensors_module_t* module,
                               struct sensor_t const**);

struct sensors_control_context_t {
    struct sensors_control_device_t device;
    /* our private state goes below here */
    int fd;
#ifdef HMC5883
    int fd1;
#endif
};

struct sensors_data_context_t {
    struct sensors_data_device_t device;
    /* our private state goes below here */
    int fd;
#ifdef HMC5883
    int fd1;
#endif

};

static struct hw_module_methods_t s_module_methods = {
    open: s_device_open
};


extern "C" const struct sensors_module_t HAL_MODULE_INFO_SYM = {
   common: {
       tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: SENSORS_HARDWARE_MODULE_ID,
        name: "OMAP Zoom sensor module",
        author: "Bosch",
        methods: &s_module_methods,
        dso:NULL,
        reserved: {},

    },

    get_sensors_list :s_get_sensors_list,
};


static struct sensor_t sensors_list[] = {
#ifdef ACCELEROMETER
    {
        name: "BMA150 Acclerometer",
        vendor: "Bosch",
        version: 1,
        handle: S_HANDLE_ACCELEROMETER,
        type: SENSOR_TYPE_ACCELEROMETER,
        maxRange: 1.0,   /* ???? */
        resolution: 1.0, /* ???? */
        power: 20,      /* ???? */
    },
#endif

#ifdef PRESSURER
	{
        name: "BMP085 Pressure",
        vendor: "Bosch",
        version: 1,
        handle: SENSORS_TEMPERATURE_HANDLE,
        type: SENSOR_TYPE_TEMPERATURE,
        maxRange: 1.0,   /* ???? */
        resolution: 1.0, /* ???? */
        power: 20,      /* ???? */
    },
#endif
#ifdef HMC5883
    {
        name: "HMC5883 Magnetic",
        vendor: "Honeywell",
        version: 1,
        handle: S_HANDLE_MAGNETIC_FIELD,
        type: SENSOR_TYPE_MAGNETIC_FIELD,
        maxRange: 1.0,   /* ???? */
        resolution: 1.0, /* ???? */
        power: 20,      /* ???? */
    },
#endif
};

static int sensors_list_size = sizeof(sensors_list) / sizeof(sensors_list[0]);

/*****************************************************************************/

static int s_dev_control_close (struct hw_device_t* device)
{
    struct sensors_control_context_t *dev;
    dev = (struct sensors_control_context_t *)device;
	printf("%s\n",__FUNCTION__);
    if (dev->fd >= 0) close (dev->fd);
#ifdef HMC5883
    if (dev->fd1 >= 0) close (dev->fd1);
#endif
    free (dev);
    return 0;
}

static native_handle_t* s_open_data_source (struct sensors_control_device_t *device)
{
    struct sensors_control_context_t *dev;
    native_handle_t * handle;
    int fd;
    dev = (struct sensors_control_context_t *)device;
    fd = open (SENSOR_DATA_DEVICE, O_RDONLY);
    if (fd >= 0) dev->fd = dup(fd);
    LOGD ("Open Data source: %d, %d\n", fd, errno);
	printf("%s\n",__FUNCTION__);
    handle = native_handle_create(2,0);
    handle->data[0] = fd;
#ifdef HMC5883
	int fd1;
    fd1 = open (MSENSOR_DATA_DEVICE, O_RDONLY);
    if (fd1 >= 0) dev->fd1 = dup(fd1);
    LOGD ("Open Data source: %d, %d\n", fd1, errno);
	printf("%s\n",__FUNCTION__);
    handle->data[1] = fd1;
#endif
    return handle;
}

static int s_activate (struct sensors_control_device_t *device,
            int handle,
            int enabled)
{
    struct sensors_control_context_t *dev;
    dev = (struct sensors_control_context_t *)device;
	printf("%s\n",__FUNCTION__);
    // ioctl here?
    return 0;
}

static int s_set_delay (struct sensors_control_device_t *device,
             int32_t ms)
{
    struct sensors_control_context_t *dev;
    dev = (struct sensors_control_context_t *)device;
    s_timeout = ms;
    LOGI("set delay to %d", ms);
	printf("%s\n",__FUNCTION__);
    // ioctl here?
    return 0;
}

static int s_wake (struct sensors_control_device_t *device)
{
    struct sensors_control_context_t *dev;
    dev = (struct sensors_control_context_t *)device;
	printf("%s\n",__FUNCTION__);
    // ioctl here?
    return 0;
}

/*****************************************************************************/

static int s_dev_data_close (struct hw_device_t* device)
{
    struct sensors_data_context_t *dev;
    dev = (struct sensors_data_context_t *)device;
    // ioctl here to quiet device?
	printf("%s\n",__FUNCTION__);
    free (dev);
    return 0;
}

static int s_data_open (struct sensors_data_device_t *device,
             native_handle_t *handle)
{
    struct sensors_data_context_t *dev;
    dev = (struct sensors_data_context_t *)device;
    dev->fd = dup(handle->data[0]);
#ifdef HMC5883
    dev->fd1 = dup(handle->data[1]);
#endif
    native_handle_close(handle);
    native_handle_delete(handle);
	printf("%s\n",__FUNCTION__);
    return 0;
}

#ifdef HMC5883
#define PI 						3.1415926535897932f
#ifndef GRAVITY_EARTH
#define	GRAVITY_EARTH           9.80665f
#endif
#define GRAVITY_SENSITIVITY		0.017f
#define GRAVITY_PARAM			0.166713f /*(GRAVITY_EARTH*GRAVITY_SENSITIVITY)*/
#define COMPASS_CALIBRATE_FILE	"/data/compass_calibration"
#define COMPASS_THRESHOLD		12.0f
static int compass_data[3];
static float orietation_data[3];
static float sensor_angle[3];



unsigned short	S_Mx_Offset			= 0x773;
unsigned short	S_My_Offset 		= 0x633;
unsigned short	S_Mx_Sensitivity	= 0xb95;
unsigned short	S_My_Sensitivity	= 0x15e8;
short 			S_Mx_Min			= 0x26;
short			S_My_Min			= 0xffd9;
static int compass_device_exist		= -1; //0:not exist, 1:exist
static int calibration_file_st_mtime = 0;
static int sensor_type = 0;
static int poll_cnt = 0;

static int Calculate_Sensor_Angle(void)
{
	float ipitch, iroll;
	float tmp = 0;
	float x, y, z;

	x = orietation_data[0];
	y = orietation_data[1];
	z = orietation_data[2];
	
	//ipitch
	if (y != 0)
	{
		tmp = z/y;
		ipitch = atan(tmp)*180/PI;
	}else
		ipitch = (z>0)?90:-90;
	
	if (z <= 0)
	{
		if (y >= 0)
			ipitch = -90 + ipitch;
		else
			ipitch = ipitch + 90;
	}else
	{
		if (y >= 0)
			ipitch = ipitch - 90;
		else
			ipitch = 90 + ipitch;
	}
	sensor_angle[1] = ((int)(ipitch*10 + 0.5))/10.0;

	//roll
	// if (x!=0)
	// {
	// 	tmp = x/z;
	// 	iroll = atan(tmp)*180/PI;
	// }else
	// 	iroll = (z>0)?90:-90;
	// if (z <= 0)
	// {
	// 	if (x >= 0)
	// 		{
	// 		iroll = -90 + iroll;
	// 		}
	// 	else
	// 		iroll = iroll + 90;
	// }else
	// {
	// 	if (x >= 0)
	// 		iroll = iroll - 90;
	// 	else
	// 		iroll = 90 + iroll ;
	// }
	if (x!=0)
	{
		tmp = x/z;
		iroll = atan(tmp)*180/PI;
	}else
		iroll = (z>0)?90:-90;
	if (z <= 0)
	{
		if (iroll <= 0){
				iroll = 90 + iroll;
		}
		else{
			iroll = iroll - 90;
		}
	}else
	{
		if (iroll <= 0){
			//iroll = -180 - iroll;
			iroll = iroll;
		}
		else{
			//iroll = iroll - 180;
			iroll = iroll;
		}
	}

	//end
	sensor_angle[2] = ((int)(iroll*10 + 0.5))/10.0;

	//	DBG("cruise  Calculate_Sensor_Angle  : ******** [Y]:%f [Z]:%f ******\n", sensor_angle[1], sensor_angle[2]);
	return 0;
}

#endif
static int s_data_close (struct sensors_data_device_t *device)
{
    struct sensors_data_context_t *dev;
    dev = (struct sensors_data_context_t *)device;
    if (dev->fd >= 0) close(dev->fd);
    dev->fd = -1;
#ifdef HMC5883
    if (dev->fd1 >= 0) close(dev->fd1);
    dev->fd1 = -1;
#endif
	printf("%s\n",__FUNCTION__);
    return 0;
}
#ifdef HMC5883
/*FIXME*/
static	int m = 1;
static int s_poll (struct sensors_data_device_t *device,
              sensors_data_t* data)
{
    struct sensors_data_context_t *dev;
	struct pollfd fds[2];
    static int count = 0;
    int magX = 0; int magY = 0; int magZ = 0;
	int offsets[3] = {0};
	int iResult;
	double dHeading = 0;
    float accForward;
    float accLeft;
	struct {
		short x;
		short y;
		short z;
	} rawData;

	struct {
		short x;
		short y;
		short z;
	} mrawData;

	//LOGE("in Sensor poll Sensortype is :%d\n",data->sensor);


	//printf("%s\n",__FUNCTION__);
    dev = (struct sensors_data_context_t *)device;
    if (dev->fd < 0)
        return 0;
    if (dev->fd1 < 0)
        return 0;

	fds[0] =  {fd: dev->fd, events: POLLIN, revents: 0};
	fds[1] =  {fd: dev->fd1, events: POLLIN, revents: 0};
	//    pollfd pfd = {fd: dev->fd, events: POLLIN, revents: 0};

	//    int err = poll (&pfd, 1, s_timeout);
	int err = poll (fds, 2,INFTIM);
	
    if (err < 0)
        return err;
    if (err == 0)
        return -EWOULDBLOCK;

    // FIXME!!!!!
    // The device currently always has data available and so
    // it can eat up the CPU!!!
    usleep(50000);	// 50ms
	//if((fds[0].revents & POLLIN) == POLLIN){
	if(((fds[0].revents & POLLIN) == POLLIN) && (m == 1)){m = 0;

		err = read (dev->fd, &rawData, sizeof(rawData));
		if (err < 0)
			return err;

		struct timespec t;
		clock_gettime(CLOCK_REALTIME, &t);

		data->time = timespec_to_ns(&t);
		data->sensor = SENSOR_TYPE_ACCELEROMETER;
		data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
		data->acceleration.x = rawData.y * CONVERT_X;
		data->acceleration.y = -rawData.x * CONVERT_Y;
		data->acceleration.z = -rawData.z * CONVERT_Z;


		//#ifdef DEBUG_SENSOR
		#if 0
		LOGE("Sensor data: t x,y,z: %d %f, %f, %f\n",
			 (int)(data->time / NSEC_PER_SEC),
			 data->acceleration.x,
			 data->acceleration.y,
			 data->acceleration.z);
#endif
		return S_HANDLE_ACCELEROMETER;

	}
	//=====================================================================================
	//	LOGE("before MAG Sensor poll:\n");
	//if((fds[1].revents & POLLIN) == POLLIN){	LOGE("in MAG Sensor poll:\n");
	if(((fds[1].revents & POLLIN) == POLLIN) && (m == 0)){m = 1;

		 read (dev->fd, &rawData, sizeof(rawData));
		 read (dev->fd1, &mrawData, sizeof(mrawData));

		// LOGE("gSensor1 data:  x,y,z:  %d, %d, %d\n",                
		// 	 rawData.x,
		// 	 rawData.y,
		// 	 rawData.z);


		// LOGE("mSensor1 data:  x,y,z:  %d, %d, %d\n",                
		// 	 mrawData.x,
		// 	 mrawData.y,
		// 	 mrawData.z);

		if (err < 0){
			return err;
		}

		orietation_data[0] = rawData.x;
		orietation_data[1] = rawData.y;
		orietation_data[2] = rawData.z;

		compass_data[0] = mrawData.x;
		compass_data[1] = mrawData.y;
		compass_data[2] = mrawData.z;

       magX = compass_data[0];
       magY = compass_data[1];
       magZ = compass_data[2];

	   count++;
       if (count < 120){
       CollectDataItem(magX, magY, magZ);
       //LOGE("CollectData.\n");
	   }

       iResult = Calibrate(&offsets[0]);
       // Elsewhere in the application, getting magnetic data and correcting out the
       // hard iron disturbances 
       magX -= offsets[0];
       magY -= offsets[1];
       magZ -= offsets[2];                                             
       //Magnetic field data is now ready to be used
	   dHeading = Heading(-magX,magY,magZ,accForward ,accLeft);

       // dHeading = atan2(magY ,magX);//FABE
	   // dHeading = dHeading*180/PI;//FABE
	   //LOGE("dHeading=%f.\n",dHeading);                                
       if (dHeading >0)
       {
       // Use the Heading Value
       //LOGE("dHeading= %lf.\n",dHeading);
       data->orientation.x = dHeading;
       //LOGE("data->orientation.x= %f.\n",data->orientation.x);
       }

       Calculate_Sensor_Angle();
	   data->sensor = SENSOR_TYPE_MAGNETIC_FIELD;
       data->orientation.y = sensor_angle[1];
       data->orientation.z = sensor_angle[2];
	   //       DBG("[cruise] Orientation  ######### x:%f y:%f z:%f #########\n", data->orientation.x,
	   //       data->orientation.y, data->orientation.z);
	   // LOGE("data->orientation.x= %f.\n",data->orientation.x);
	   // LOGE("data->orientation.y= %f.\n",data->orientation.y);
	   // LOGE("data->orientation.z= %f.\n",data->orientation.z);
	   // LOGE("sensor_type\n",sensor_type);

		// struct timespec t;
		// clock_gettime(CLOCK_REALTIME, &t);

		// data->time = timespec_to_ns(&t);
		// data->sensor = SENSOR_TYPE_MAGNETIC_FIELD;
		// data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
		// data->acceleration.x = rawData.y * CONVERT_X;
		// data->acceleration.y = -rawData.x * CONVERT_Y;
		// data->acceleration.z = -rawData.z * CONVERT_Z;

#ifdef DEBUG_SENSOR
		LOGI("Sensor data: t x,y,x: %d %f, %f, %f\n",
			 (int)(data->time / NSEC_PER_SEC),
			 data->acceleration.x,
			 data->acceleration.y,
			 data->acceleration.z);
#endif
		return S_HANDLE_MAGNETIC_FIELD;
	}
		return err;
}
#else
static int s_poll (struct sensors_data_device_t *device,
              sensors_data_t* data)
{
    struct sensors_data_context_t *dev;
	//printf("%s\n",__FUNCTION__);
    dev = (struct sensors_data_context_t *)device;

    if (dev->fd < 0)
        return 0;
    pollfd pfd = {fd: dev->fd, events: POLLIN, revents: 0};
	//    int err = poll (&pfd, 1, s_timeout);
	int err = poll (&pfd, 1, INFTIM);
    if (err < 0)
        return err;
    if (err == 0)
        return -EWOULDBLOCK;

    // FIXME!!!!!
    // The device currently always has data available and so
    // it can eat up the CPU!!!
    usleep(50000);	// 50ms

#ifdef  ACCELEROMETER
#ifdef	BMA150_SENSOR
    struct {
        short x;
        short y;
        short z;
    } rawData;
#endif

#ifdef BMA120_SENSOR
	struct {
        signed char x;
        signed char y;
        signed char z;
    } rawData;
#endif

	if((pfd.revents & POLLIN) == POLLIN){
		err = read (dev->fd, &rawData, sizeof(rawData));
	}
    if (err < 0)
        return err;

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);

    data->time = timespec_to_ns(&t);
    data->sensor = SENSOR_TYPE_ACCELEROMETER;
    data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
    data->acceleration.x = rawData.y * CONVERT_X;
    data->acceleration.y = -rawData.x * CONVERT_Y;
    data->acceleration.z = -rawData.z * CONVERT_Z;

#ifdef DEBUG_SENSOR
    LOGI("Sensor data: t x,y,x: %d %f, %f, %f\n",
                (int)(data->time / NSEC_PER_SEC),
                data->acceleration.x,
                data->acceleration.y,
                data->acceleration.z);
#endif
    return S_HANDLE_ACCELEROMETER;
#endif

#ifdef PRESSURER
	struct{
		short	temperature;
		long	pressure;
	}rawData;
	err = read(dev->fd, &rawData, sizeof(rawData));
	if(err < 0)
		return err;

	struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);

    data->time = timespec_to_ns(&t);
    data->sensor = SENSOR_TYPE_ACCELEROMETER;

    data->vector.x = -rawData.temperature;
    data->vector.y = -rawData.pressure;
    data->vector.z = 0;
	LOGD("Sensor data: t x,y,x: %d %f, %f, %f\n",
                (int)(data->time / NSEC_PER_SEC),
                data->vector.x,
                data->vector.y,
                data->vector.z);
	return S_HANDLE_PRESSURER;
#endif
}
#endif

/*****************************************************************************/

static int s_get_sensors_list (struct sensors_module_t* module,
                    struct sensor_t const** list)
{
    *list = sensors_list;
	printf("%s\n",__FUNCTION__);
    return sensors_list_size;
}

/*****************************************************************************/

static int s_device_open(const struct hw_module_t* module,
              const char* name,
              struct hw_device_t** device)
{
    int status = -EINVAL;
	printf("%s\n",__FUNCTION__);
    if (!strcmp(name, SENSORS_HARDWARE_CONTROL)) {
        sensors_control_context_t *dev;
        dev = (sensors_control_context_t *)malloc(sizeof(*dev));
        if (! dev) return status;

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = (struct hw_module_t *)module;
        dev->device.common.close = s_dev_control_close;

        dev->device.open_data_source = s_open_data_source;
        dev->device.activate = s_activate;
        dev->device.set_delay = s_set_delay;
        dev->device.wake = s_wake;

        *device = &dev->device.common;
        status = 0;
		printf("%d\n",__LINE__);
    } else if (!strcmp(name, SENSORS_HARDWARE_DATA)) {
        sensors_data_context_t *dev;
        dev = (sensors_data_context_t *)malloc(sizeof(*dev));
        if (! dev) return status;

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = (struct hw_module_t *)module;
        dev->device.common.close = s_dev_data_close;

        dev->device.data_open = s_data_open;
        dev->device.data_close = s_data_close;
        dev->device.poll = s_poll;
        dev->fd = -1;
        dev->fd1 = -1;


        *device = &dev->device.common;
        status = 0;
		printf("%d\n",__LINE__);
    }
    return status;
}


