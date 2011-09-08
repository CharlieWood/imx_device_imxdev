/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "Sensors"

#define LOG_NDEBUG 1

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <sys/select.h>
#include <stdlib.h>

#include <linux/input.h>
#include <linux/akm8973.h>
#include <linux/capella_cm3602.h>
#include <linux/lightsensor.h>
#include <linux/hmc5883.h>

#include <cutils/atomic.h>
#include <cutils/log.h>
#include <cutils/native_handle.h>

#define __MAX(a,b) ((a)>=(b)?(a):(b))

#define HMC5883

#ifdef HMC5883
#include <HMSHardIronCal.h>
#include <HMSHeading.h>
#define PI 						3.1415926535897932f
#endif

/*****************************************************************************/

#define MAX_NUM_SENSORS 6

#define SUPPORTED_SENSORS  ((1<<MAX_NUM_SENSORS)-1)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_M  (1)
#define ID_O  (2)
#define ID_T  (3)
#define ID_P  (4)
#define ID_L  (5)

static int id_to_sensor[MAX_NUM_SENSORS] = {
    [ID_A] = SENSOR_TYPE_ACCELEROMETER,
    [ID_M] = SENSOR_TYPE_MAGNETIC_FIELD,
    [ID_O] = SENSOR_TYPE_ORIENTATION,
    [ID_T] = SENSOR_TYPE_TEMPERATURE,
    [ID_P] = SENSOR_TYPE_PROXIMITY,
    [ID_L] = SENSOR_TYPE_LIGHT,
};

#define SENSORS_ACCELERATION   (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD (1<<ID_M)
#define SENSORS_ORIENTATION    (1<<ID_O)
#define SENSORS_TEMPERATURE    (1<<ID_T)
#define SENSORS_AKM_GROUP          ((1<<ID_A)|(1<<ID_M)|(1<<ID_O)|(1<<ID_T))

#define SENSORS_CM_PROXIMITY       (1<<ID_P)
#define SENSORS_CM_GROUP           (1<<ID_P)

#define SENSORS_LIGHT              (1<<ID_L)
#define SENSORS_LIGHT_GROUP        (1<<ID_L)

/*****************************************************************************/

struct sensors_control_context_t {
    struct sensors_control_device_t device; // must be first
    int accd_fd;
    int orid_fd;
    uint32_t active_sensors;
};

struct sensors_data_context_t {
    struct sensors_data_device_t device; // must be first
    int events_fd[2];
    sensors_data_t sensors[MAX_NUM_SENSORS];
    uint32_t pendingSensors;
};

struct {
	short x;
	short y;
	short z;
} mrawData;

static char buff[32] = {0};
static int delay_time_ms = 0;

/*
 * The SENSORS Module
 */

/* the CM3602 is a binary proximity sensor that triggers around 9 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_CM  9.0f

/*
 * the AK8973 has a 8-bit ADC but the firmware seems to average 16 samples,
 * or at least makes its calibration on 12-bits values. This increases the
 * resolution by 4 bits.
 */

static const struct sensor_t sSensorList[] = {
        { "kxtf9 3-axis Accelerometer",
                "Kionix",
                1, SENSORS_HANDLE_BASE+ID_A,
                SENSOR_TYPE_ACCELEROMETER, 4.0f*9.81f, (4.0f*9.81f)/256.0f, 0.2f, 0, { } },
	{ "HMC5883 Magnetic field sensor",
		"Honeywell",
		1, SENSORS_HANDLE_BASE+ID_M,
		SENSOR_TYPE_MAGNETIC_FIELD, 2000.0f, 1.0f/16.0f, 6.8f, 0, { } }, 
        { "HMC5883 Magnetic field sensor",
                "Honeywell",
                1, SENSORS_HANDLE_BASE+ID_O,
                SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f, 7.0f, 0, { } },
        /*{ "ISL29018 Light sensor",
                "Intersil",
                1, SENSORS_HANDLE_BASE+ID_L,
                SENSOR_TYPE_LIGHT, 10240.0f, 1.0f, 0.5f, 0, { } },
        */
};

static const float sLuxValues[8] = {
    10.0,
    160.0,
    225.0,
    320.0,
    640.0,
    1280.0,
    2600.0,
    10240.0
};

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

const struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "kxtf9 & hmc5883",
        .author = "The Android Open Source Project",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = sensors__get_sensors_list
};

/*****************************************************************************/

#define ACC_DEVICE_NAME		"/dev/input/event4"
#define ORI_DEVICE_NAME		"/dev/hmc5883"
//#define LS_DEVICE_NAME      "/sys/class/lightsensor/switch_cmd/lightsensor_file_cmd"


// sensor IDs must be a power of two and
// must match values in SensorManager.java
#define EVENT_TYPE_ACCEL_X          ABS_X
#define EVENT_TYPE_ACCEL_Y          ABS_Z
#define EVENT_TYPE_ACCEL_Z          ABS_Y
#define EVENT_TYPE_ACCEL_STATUS     ABS_WHEEL

#define EVENT_TYPE_YAW              ABS_RX
#define EVENT_TYPE_PITCH            ABS_RY
#define EVENT_TYPE_ROLL             ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS    ABS_RUDDER

#define EVENT_TYPE_MAGV_X           ABS_HAT0X
#define EVENT_TYPE_MAGV_Y           ABS_HAT0Y
#define EVENT_TYPE_MAGV_Z           ABS_BRAKE

#define EVENT_TYPE_TEMPERATURE      ABS_THROTTLE
#define EVENT_TYPE_STEP_COUNT       ABS_GAS
#define EVENT_TYPE_PROXIMITY        ABS_DISTANCE
#define EVENT_TYPE_LIGHT            ABS_MISC

// 720 LSG = 1G
//#define LSG                         (720.0f)


// conversion of acceleration data to SI units (m/s^2)
//#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A                   (GRAVITY_EARTH / 1000)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// conversion of magnetic data to uT units
#define CONVERT_M                   (1.0f/16.0f)
#define CONVERT_M_X                 (-CONVERT_M)
#define CONVERT_M_Y                 (-CONVERT_M)
#define CONVERT_M_Z                 (CONVERT_M)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

static int open_inputs(int mode, int *acc_fd, int *ori_fd)
{
    /* scan all input drivers and look for "compass" */
    int fd = -1;
    const char *dirname = "/dev/input";
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;

    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    *acc_fd = *ori_fd = -1;
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        fd = open(devname, mode);
        if (fd>=0) {
            char name[80];
            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                name[0] = '\0';
            }
		if (!strcmp(name, "kxtf9_accel")) {
                LOGD("using %s (name=%s)", devname, name);
                *acc_fd = fd;
            }
            else if (!strcmp(name, "proximity")) {
                LOGD("using %s (name=%s)", devname, name);
                *ori_fd = fd;
            }
            /*else if (!strcmp(name, "lightsensor")) {
                LOGD("using %s (name=%s)", devname, name);
                *l_fd = fd;
            }*/
            else
                close(fd);
        }
    }
    closedir(dir);

	fd = open(ORI_DEVICE_NAME, O_RDONLY);
	if(fd>=0)
		{
			*ori_fd = fd;
		}
    fd = 0;
    if (*acc_fd < 0) {
        LOGE("Couldn't find or open 'compass' driver (%s)", strerror(errno));
        fd = -1;
    }
    if (*ori_fd < 0) {
        LOGE("Couldn't find or open 'proximity' driver (%s)", strerror(errno));
        fd = -1;
    }
    /*if (*l_fd < 0) {
        LOGE("Couldn't find or open 'light' driver (%s)", strerror(errno));
        fd = -1;
    }*/
    return fd;
}

static int open_acc(struct sensors_control_context_t* dev)
{
    if (dev->accd_fd < 0) {
        dev->accd_fd = open(ACC_DEVICE_NAME, O_RDONLY);
        LOGV("%s, fd=%d", __PRETTY_FUNCTION__, dev->accd_fd);
        LOGE_IF(dev->accd_fd<0, "Couldn't open %s (%s)",
                ACC_DEVICE_NAME, strerror(errno));
        if (dev->accd_fd >= 0) {
            dev->active_sensors &= ~SENSORS_AKM_GROUP;
        }
    }
    return dev->accd_fd;
}

static void close_acc(struct sensors_control_context_t* dev)
{
    if (dev->accd_fd >= 0) {
        LOGV("%s, fd=%d", __PRETTY_FUNCTION__, dev->accd_fd);
        close(dev->accd_fd);
        dev->accd_fd = -1;
    }
}

static uint32_t read_acc_sensors_state(int fd)
{
    short flags;
    uint32_t sensors = 0;
    // read the actual value of all sensors
    /* if (!ioctl(fd, ECS_IOCTL_APP_GET_MFLAG, &flags)) { */
    /*     if (flags)  sensors |= SENSORS_ORIENTATION; */
    /*     else        sensors &= ~SENSORS_ORIENTATION; */
    /* } */
    /* if (!ioctl(fd, ECS_IOCTL_APP_GET_AFLAG, &flags)) { */
    /*     if (flags)  sensors |= SENSORS_ACCELERATION; */
    /*     else        sensors &= ~SENSORS_ACCELERATION; */
    /* } */
    /* if (!ioctl(fd, ECS_IOCTL_APP_GET_TFLAG, &flags)) { */
    /*     if (flags)  sensors |= SENSORS_TEMPERATURE; */
    /*     else        sensors &= ~SENSORS_TEMPERATURE; */
    /* } */
    /* if (!ioctl(fd, ECS_IOCTL_APP_GET_MVFLAG, &flags)) { */
    /*     if (flags)  sensors |= SENSORS_MAGNETIC_FIELD; */
    /*     else        sensors &= ~SENSORS_MAGNETIC_FIELD; */
    /* } */

	sensors |= SENSORS_ACCELERATION;

    return sensors;
}

static uint32_t enable_disable_acc(struct sensors_control_context_t *dev,
                                   uint32_t active, uint32_t sensors,
                                   uint32_t mask)
{
    uint32_t now_active_acc_sensors;
    //FIXME
    now_active_acc_sensors = 0;
    now_active_acc_sensors |= SENSORS_ACCELERATION;
    return now_active_acc_sensors;
    //
    int fd = open_acc(dev);
    if (fd < 0)
        return 0;

    LOGE("(before) acc sensors = %08x, real = %08x",
         sensors, read_acc_sensors_state(fd));

    short flags;
    if (mask & SENSORS_ORIENTATION) {
        flags = (sensors & SENSORS_ORIENTATION) ? 1 : 0;
        if (ioctl(fd, ECS_IOCTL_APP_SET_MFLAG, &flags) < 0) {
            LOGE("ECS_IOCTL_APP_SET_MFLAG error (%s)", strerror(errno));
        }
    }
    if (mask & SENSORS_ACCELERATION) {
        flags = (sensors & SENSORS_ACCELERATION) ? 1 : 0;
        if (ioctl(fd, ECS_IOCTL_APP_SET_AFLAG, &flags) < 0) {
            LOGE("ECS_IOCTL_APP_SET_AFLAG error (%s)", strerror(errno));
        }
    }
    if (mask & SENSORS_TEMPERATURE) {
        flags = (sensors & SENSORS_TEMPERATURE) ? 1 : 0;
        if (ioctl(fd, ECS_IOCTL_APP_SET_TFLAG, &flags) < 0) {
            LOGE("ECS_IOCTL_APP_SET_TFLAG error (%s)", strerror(errno));
        }
    }
    if (mask & SENSORS_MAGNETIC_FIELD) {
        flags = (sensors & SENSORS_MAGNETIC_FIELD) ? 1 : 0;
        if (ioctl(fd, ECS_IOCTL_APP_SET_MVFLAG, &flags) < 0) {
            LOGE("ECS_IOCTL_APP_SET_MVFLAG error (%s)", strerror(errno));
        }
    }

    now_active_acc_sensors = read_acc_sensors_state(fd);

    LOGV("(after) acc sensors = %08x, real = %08x",
         sensors, now_active_acc_sensors);

    if (!sensors)
        close_acc(dev);

    return now_active_acc_sensors;
}

static uint32_t read_ori_sensors_state(int fd)
{
    int flags;
    uint32_t sensors = 0;

    // read the actual value of all sensors
    if (!ioctl(fd, ORIENTATION_GET_STATUS, &flags)) {
        if (flags)  sensors |= SENSORS_ORIENTATION;
        else        sensors &= ~SENSORS_ORIENTATION;
    }

    return sensors;
}

static int open_ori(struct sensors_control_context_t* dev)
{
    if (dev->orid_fd < 0) {
        dev->orid_fd = open(ORI_DEVICE_NAME, O_RDONLY);
        LOGV("%s, fd=%d", __PRETTY_FUNCTION__, dev->orid_fd);
        LOGE_IF(dev->orid_fd<0, "Couldn't open %s (%s)",
                ORI_DEVICE_NAME, strerror(errno));
        if (dev->orid_fd >= 0) {
            dev->active_sensors &= ~SENSORS_CM_GROUP;
        }
    }
    return dev->orid_fd;
}

static void close_ori(struct sensors_control_context_t* dev)
{
    if (dev->orid_fd >= 0) {
        LOGV("%s, fd=%d", __PRETTY_FUNCTION__, dev->orid_fd);
        close(dev->orid_fd);
        dev->orid_fd = -1;
    }
}

static int enable_disable_ori(struct sensors_control_context_t *dev,
                             uint32_t active, uint32_t sensors, uint32_t mask)
{
    int rc = 0;
    uint32_t now_active_ori_sensors;
    int fd = open_ori(dev);

    if (fd < 0) {
        LOGE("Couldn't open %s (%s)", ORI_DEVICE_NAME, strerror(errno));
        return 0;
    }

    LOGV("(before) ori sensors = %08x, real = %08x",
         sensors, read_ori_sensors_state(fd));

    if (mask & SENSORS_ORIENTATION) {
        int flags = (sensors & SENSORS_ORIENTATION) ? 1 : 0;
		rc = ioctl(fd, ORIENTATION_SET_STATUS, &flags);
        if (rc < 0)
            LOGE("CAPELLA_CM3602_IOCTL_ENABLE error (%s)", strerror(errno));
    }

    now_active_ori_sensors = read_ori_sensors_state(fd);

    LOGV("(after) ori sensors = %08x, real = %08x",
         sensors, now_active_ori_sensors);

    return now_active_ori_sensors;
}
#if 0
static uint32_t read_ls_sensors_state(int fd)
{
    int flags;
    uint32_t sensors = 0;
    int nread;
    char buf[256];

    memset(buf, 0, sizeof( buf ) );
    lseek( fd, 0, SEEK_SET );   
    nread = read(fd, buf, sizeof(buf));
    LOGV("read_ls_sensors_state read %d bytes, fd:%d", nread,fd );
    if( nread < 0 )
    {
        LOGE("Read light sensor status failed(%d).", errno);
        return 0;
    }
    else if( nread >= sizeof( buf ) )
    {
        LOGE("Invalid light sensor status lenght.");
        return 0;
    }
    
    sscanf( buf, "%x\n", &flags );

    LOGV("read_ls_sensors_state read flag:%d, buf(\"%s\").", flags, buf );
    if (flags)  
        sensors |= SENSORS_LIGHT;
    else        
        sensors &= ~SENSORS_LIGHT; 

    return sensors;
}

static int open_ls(struct sensors_control_context_t* dev)
{
    if (dev->lsd_fd < 0) {
        dev->lsd_fd = open(LS_DEVICE_NAME, O_RDWR);
        LOGV("%s, fd=%d", __PRETTY_FUNCTION__, dev->lsd_fd);
        LOGE_IF(dev->lsd_fd<0, "Couldn't open %s (%s)",
                LS_DEVICE_NAME, strerror(errno));
        if (dev->lsd_fd >= 0) {
            dev->active_sensors &= ~SENSORS_LIGHT_GROUP;
        }
    }
    LOGD("open_ls return lsd_fd:%d",dev->lsd_fd);    
    return dev->lsd_fd;
}

static void close_ls(struct sensors_control_context_t* dev)
{
    LOGD("close_ls for  lsd_fd:%d",dev->lsd_fd );    
    if (dev->lsd_fd >= 0) {
        LOGV("%s, fd=%d", __PRETTY_FUNCTION__, dev->lsd_fd);
        close(dev->lsd_fd);
        dev->lsd_fd = -1;
    }
}

static int enable_disable_ls(struct sensors_control_context_t *dev,
                             uint32_t active, uint32_t sensors, uint32_t mask)
{
    int ret = 0;
    uint32_t now_active_ls_sensors;
    char buf[256];

    int fd = open_ls(dev);
    LOGD("Enter enable_disable_ls, active:0x%x, sensors:0x%x, mask:0x%x",
         active& SENSORS_LIGHT, sensors& SENSORS_LIGHT, mask& SENSORS_LIGHT );
    
    if (fd < 0) {
        LOGE("Couldn't open %s (%s)", LS_DEVICE_NAME, strerror(errno));
        return 0;
    }

    LOGD("(before) ls sensors = %08x, real = %08x",
         sensors, read_ls_sensors_state(fd));

    if (mask & SENSORS_LIGHT) {
        sprintf(buf, "%x", (sensors & SENSORS_LIGHT) ? 1 : 0);
        ret = write(fd, buf , strlen( buf ) + 1 );
        if (ret < 0)
            LOGE("write to light sensor error (%s)", strerror(errno));
        LOGD("write lightsensor(%s) return %d", buf, ret );
    }

    now_active_ls_sensors = read_ls_sensors_state(fd);

    LOGD("(after) ls sensors = %08x, real = %08x",
         sensors, now_active_ls_sensors);

    return now_active_ls_sensors;
}
#endif
/*****************************************************************************/

static native_handle_t* control__open_data_source(struct sensors_control_context_t *dev)
{
    native_handle_t* handle;
    int acc_fd, ori_fd;

    if (open_inputs(O_RDONLY, &acc_fd, &ori_fd) < 0 ||
            acc_fd < 0 || ori_fd < 0) {
        LOGE("open_inputs return failed." );
        return NULL;
    }

    handle = native_handle_create(2, 0);
    handle->data[0] = acc_fd;
    handle->data[1] = ori_fd;
    //handle->data[2] = l_fd;

    return handle;
}

static int control__activate(struct sensors_control_context_t *dev,
        int handle, int enabled)
{
    if ((handle < SENSORS_HANDLE_BASE) ||
            (handle >= SENSORS_HANDLE_BASE+MAX_NUM_SENSORS))
        return -1;

    uint32_t mask = (1 << handle);
    uint32_t sensors = enabled ? mask : 0;

    uint32_t active = dev->active_sensors;
    uint32_t new_sensors = (active & ~mask) | (sensors & mask);
    uint32_t changed = active ^ new_sensors;

    if (changed) {
        if (!active && new_sensors)
            // force all sensors to be updated
            changed = SUPPORTED_SENSORS;

        dev->active_sensors =
            enable_disable_acc(dev,
                               active & SENSORS_AKM_GROUP,
                               new_sensors & SENSORS_AKM_GROUP,
                               changed & SENSORS_AKM_GROUP) |
            enable_disable_ori(dev,
                              active & SENSORS_ORIENTATION,
                              new_sensors & SENSORS_ORIENTATION,
                              changed & SENSORS_ORIENTATION); 
            /*enable_disable_ls(dev,
                              active & SENSORS_LIGHT_GROUP,
                              new_sensors & SENSORS_LIGHT_GROUP,
                              changed & SENSORS_LIGHT_GROUP);*/
    }

    return 0;
}

static int control__set_delay(struct sensors_control_context_t *dev, int32_t ms)
{
    delay_time_ms = ms;	
#ifdef ECS_IOCTL_APP_SET_DELAY
    if (dev->accd_fd <= 0) {
        return -1;
    }
    short delay = ms;
    if (!ioctl(dev->accd_fd, ECS_IOCTL_APP_SET_DELAY, &delay)) {
        return -errno;
    }
    return 0;
#else
    return -1;
#endif
}

static int control__wake(struct sensors_control_context_t *dev)
{
    int err = 0;
    int acc_fd, ori_fd;
    if (open_inputs(O_RDWR, &acc_fd, &ori_fd) < 0 ||
            acc_fd < 0 || ori_fd < 0) {
        return -1;
    }

    struct input_event event[1];
    event[0].type = EV_SYN;
    event[0].code = SYN_CONFIG;
    event[0].value = 0;

    err = write(acc_fd, event, sizeof(event));
    LOGV_IF(err<0, "control__wake(accelerometer), fd=%d (%s)",
            acc_fd, strerror(errno));
    close(acc_fd);

    err = write(ori_fd, event, sizeof(event));
    LOGV_IF(err<0, "control__wake(compass), fd=%d (%s)",
            ori_fd, strerror(errno));
    close(ori_fd);

    /*err = write(l_fd, event, sizeof(event));
    LOGV_IF(err<0, "control__wake(light), fd=%d (%s)",
            l_fd, strerror(errno));
    close(l_fd);*/

    return err;
}

/*****************************************************************************/

static int data__data_open(struct sensors_data_context_t *dev, native_handle_t* handle)
{
    int i;
    struct input_absinfo absinfo;
    memset(&dev->sensors, 0, sizeof(dev->sensors));

    for (i = 0; i < MAX_NUM_SENSORS; i++) {
        // by default all sensors have high accuracy
        // (we do this because we don't get an update if the value doesn't
        // change).
        dev->sensors[i].vector.status = SENSOR_STATUS_ACCURACY_HIGH;
    }

    dev->sensors[ID_A].sensor = SENSOR_TYPE_ACCELEROMETER;
    dev->sensors[ID_M].sensor = SENSOR_TYPE_MAGNETIC_FIELD;
    dev->sensors[ID_O].sensor = SENSOR_TYPE_ORIENTATION;
    dev->sensors[ID_T].sensor = SENSOR_TYPE_TEMPERATURE;
    dev->sensors[ID_P].sensor = SENSOR_TYPE_PROXIMITY;
    dev->sensors[ID_L].sensor = SENSOR_TYPE_LIGHT;

    dev->events_fd[0] = dup(handle->data[0]);
    dev->events_fd[1] = dup(handle->data[1]);
    //dev->events_fd[2] = dup(handle->data[2]);
    LOGV("data__data_open: accelerometer fd = %d, dup=%d", handle->data[0], dev->events_fd[0] );
    LOGV("data__data_open: compass fd = %d, dup=%d", handle->data[1], dev->events_fd[1] );
    //LOGV("data__data_open: light fd = %d, dup=%d", handle->data[2], dev->events_fd[2] );
    // Framework will close the handle
    native_handle_delete(handle);

    dev->pendingSensors = 0;
    /* if (!ioctl(dev->events_fd[1], EVIOCGABS(ABS_DISTANCE), &absinfo)) { */
    /*     LOGV("proximity sensor initial value %d\n", absinfo.value); */
    /*     dev->pendingSensors |= SENSORS_CM_PROXIMITY; */
    /*     // FIXME: we should save here absinfo.{minimum, maximum, etc} */
    /*     //        and use them to scale the return value according to */
    /*     //        the sensor description. */
    /*     dev->sensors[ID_P].distance = (float)absinfo.value; */
    /* } */
    /* else LOGE("Cannot get proximity sensor initial value: %s\n", */
              /* strerror(errno)); */

    return 0;
}

static int data__data_close(struct sensors_data_context_t *dev)
{
    if (dev->events_fd[0] >= 0) {
        //LOGV("(data close) about to close compass fd=%d", dev->events_fd[0]);
        close(dev->events_fd[0]);
        dev->events_fd[0] = -1;
    }
    if (dev->events_fd[1] >= 0) {
        //LOGV("(data close) about to close proximity fd=%d", dev->events_fd[1]);
        close(dev->events_fd[1]);
        dev->events_fd[1] = -1;
    }
    /*if (dev->events_fd[2] >= 0) {
        //LOGV("(data close) about to close light fd=%d", dev->events_fd[1]);
        close(dev->events_fd[2]);
        dev->events_fd[2] = -1;
    }*/
    return 0;
}

static int pick_sensor(struct sensors_data_context_t *dev,
        sensors_data_t* values)
{
    uint32_t mask = SUPPORTED_SENSORS;
    while (mask) {
        uint32_t i = 31 - __builtin_clz(mask);
        mask &= ~(1<<i);
        if (dev->pendingSensors & (1<<i)) {
            dev->pendingSensors &= ~(1<<i);
            *values = dev->sensors[i];
            values->sensor = id_to_sensor[i];
            LOGV_IF(0, "%d [%f, %f, %f]",
                    values->sensor,
                    values->vector.x,
                    values->vector.y,
                    values->vector.z);
#if 0
            LOGE("_______________sensor is:%d [%f, %f, %f],i is:%d", 
                    values->sensor, 
                    values->vector.x, 
                    values->vector.y, 
				 values->vector.z, 
				 i); 
#endif
			return i;
        }
    }

    LOGE("no sensor to return: pendingSensors = %08x", dev->pendingSensors);
    return -1;
}
//
struct compass_data_t {
	short x;
	short y;
	short z;
};

static struct compass_data_t g_compass_data;
static long int g_ls_data;
static int gsensor_data[3];
static int compass_data[3];
static float orietation_data[3];
static float sensor_angle[3];

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
		}
    else
		ipitch = (z>0)?90:-90;
	
	if (z <= 0)
		{
			if (y >= 0)
				ipitch = 90 + ipitch;
			else
				ipitch = ipitch - 90;
		}
    else
		{
			if (y >= 0)
				ipitch = ipitch - 90;
			else
				ipitch = 90 + ipitch;
		}
	sensor_angle[1] = ((int)(ipitch*10 + 0.5))/10.0;

	// roll
	if (x!=0)
		{
			tmp = z/x;
			iroll = atan(tmp)*180/PI;
		}
	else
		iroll = (z>0)?90:-90;

	if (z <= 0)
		{
			if (x >= 0)			
				iroll = iroll + 90;			
			else
				iroll = iroll - 90;
		}
	else
		{
			if (x >= 0)
				iroll = iroll + 90 ;
			else
				iroll = iroll - 90 ;
		}

	sensor_angle[2] = ((int)(iroll*10 + 0.5))/10.0;

	//	DBG("cruise  Calculate_Sensor_Angle  : ******** [Y]:%f [Z]:%f ******\n", sensor_angle[1], sensor_angle[2]);
	return 0;
}

static void calculate_compass()
{
	int magX = 0; int magY = 0; int magZ = 0;
	static int count = 0;
	int iResult;
	int offsets[3] = {0};
	double dHeading = 0;
    float accForward;
    float accLeft;

	orietation_data[0] = gsensor_data[0];
	orietation_data[1] = gsensor_data[1];
	orietation_data[2] = gsensor_data[2];

	compass_data[0] = g_compass_data.y;
	compass_data[1] = -g_compass_data.x;
	compass_data[2] = g_compass_data.z;

	magX = compass_data[0];
	magY = compass_data[1];
	magZ = compass_data[2];

	count++;
	if (count < 1200){
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
	accForward = 0;
	accLeft = 0;	
	dHeading = Heading(-magX,magY,magZ,accForward ,accLeft);

	if (dHeading >0)
		{
			// Use the Heading Value
			g_compass_data.x = dHeading;
		}

	Calculate_Sensor_Angle();
	g_compass_data.y = sensor_angle[1];
	g_compass_data.z = sensor_angle[2];

}

//

static uint32_t data__poll_process_acc_abs(struct sensors_data_context_t *dev,
                                           int fd __attribute__((unused)),
                                           struct input_event *event)
{
    uint32_t new_sensors = 0;
    if (event->type == EV_ABS) {
        switch (event->code) {
        case EVENT_TYPE_ACCEL_X:
            new_sensors |= SENSORS_ACCELERATION;
            dev->sensors[ID_A].acceleration.y = event->value * CONVERT_A_Y;
			gsensor_data[1] = dev->sensors[ID_A].acceleration.y;
            break;
        case EVENT_TYPE_ACCEL_Z:
            new_sensors |= SENSORS_ACCELERATION;
            dev->sensors[ID_A].acceleration.x = event->value * CONVERT_A_X;
			gsensor_data[0] = dev->sensors[ID_A].acceleration.x; 
            break;
        case EVENT_TYPE_ACCEL_Y:
            new_sensors |= SENSORS_ACCELERATION;
            dev->sensors[ID_A].acceleration.z = event->value * CONVERT_A_Z;
			gsensor_data[2] = dev->sensors[ID_A].acceleration.z; 
            break;
        case EVENT_TYPE_MAGV_X:
            new_sensors |= SENSORS_MAGNETIC_FIELD;
            dev->sensors[ID_M].magnetic.x = event->value * CONVERT_M_X;
            break;
        case EVENT_TYPE_MAGV_Y:
            new_sensors |= SENSORS_MAGNETIC_FIELD;
            dev->sensors[ID_M].magnetic.y = event->value * CONVERT_M_Y;
            break;
        case EVENT_TYPE_MAGV_Z:
            new_sensors |= SENSORS_MAGNETIC_FIELD;
            dev->sensors[ID_M].magnetic.z = event->value * CONVERT_M_Z;
            break;
        case EVENT_TYPE_YAW:
            new_sensors |= SENSORS_ORIENTATION;
            dev->sensors[ID_O].orientation.azimuth =  event->value;
            break;
        case EVENT_TYPE_PITCH:
            new_sensors |= SENSORS_ORIENTATION;
            dev->sensors[ID_O].orientation.pitch = event->value;
            break;
        case EVENT_TYPE_ROLL:
            new_sensors |= SENSORS_ORIENTATION;
            dev->sensors[ID_O].orientation.roll = -event->value;
            break;
        case EVENT_TYPE_TEMPERATURE:
            new_sensors |= SENSORS_TEMPERATURE;
            dev->sensors[ID_T].temperature = event->value;
            break;
        case EVENT_TYPE_STEP_COUNT:
            // step count (only reported in MODE_FFD)
            // we do nothing with it for now.
            break;
        case EVENT_TYPE_ACCEL_STATUS:
            // accuracy of the calibration (never returned!)
            //LOGV("G-Sensor status %d", event->value);
            break;
        case EVENT_TYPE_ORIENT_STATUS: {
            // accuracy of the calibration
            uint32_t v = (uint32_t)(event->value & SENSOR_STATE_MASK);
            LOGV_IF(dev->sensors[ID_O].orientation.status != (uint8_t)v,
                    "M-Sensor status %d", v);
            dev->sensors[ID_O].orientation.status = (uint8_t)v;
        }
            break;
        }
    }

    return new_sensors;
}

static uint32_t data__poll_process_ori_abs(struct sensors_data_context_t *dev,
                                           int fd __attribute__((unused)))
{
    uint32_t new_sensors = 0;

	dev->sensors[ID_M].magnetic.x = g_compass_data.x;
	dev->sensors[ID_M].magnetic.y = g_compass_data.y;
	dev->sensors[ID_M].magnetic.z = g_compass_data.z;

	calculate_compass();
	dev->sensors[ID_O].orientation.azimuth = g_compass_data.x;
	dev->sensors[ID_O].orientation.pitch = g_compass_data.y;
	dev->sensors[ID_O].orientation.roll = g_compass_data.z;
	/* LOGE("x is: %f",	dev->sensors[ID_O].orientation.azimuth); */
	/* LOGE("y is: %f",	dev->sensors[ID_O].orientation.pitch); */
	/* LOGE("z is: %f",	dev->sensors[ID_O].orientation.roll); */
	new_sensors |= SENSORS_ORIENTATION;
	new_sensors |= SENSORS_MAGNETIC_FIELD;

    return new_sensors;
}
#if 0
static uint32_t data__poll_process_ls_abs(struct sensors_data_context_t *dev,
                                          int fd __attribute__((unused)),
                                          struct input_event *event)
{
    uint32_t new_sensors = 0;
     if (event->type == EV_ABS) { 
         LOGV("lightsensor type: %d code: %d value: %-5d time: %ds", 
              event->type, event->code, event->value, 
              (int)event->time.tv_sec); 
         if (event->code == EVENT_TYPE_LIGHT) { 
              new_sensors |= SENSORS_LIGHT; 
              dev->sensors[ID_L].light = event->value; 
              
         } 
     } 

    return new_sensors;
}
#endif
static void data__poll_process_syn_not_inputdev(struct sensors_data_context_t *dev,
                                   uint32_t new_sensors)
{
    struct timespec  ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (new_sensors) {
        dev->pendingSensors |= new_sensors;
        /* int64_t t = event->time.tv_sec*1000000000LL + */
        /*     event->time.tv_usec*1000; */
        while (new_sensors) {
            uint32_t i = 31 - __builtin_clz(new_sensors);
            new_sensors &= ~(1<<i);
            dev->sensors[i].time = ts.tv_sec * 1000000 + ts.tv_nsec/1000;
        }
    }
}

static void data__poll_process_syn(struct sensors_data_context_t *dev,
                                   struct input_event *event,
                                   uint32_t new_sensors)
{
    if (new_sensors) {
        dev->pendingSensors |= new_sensors;
        int64_t t = event->time.tv_sec*1000000000LL +
            event->time.tv_usec*1000;
        while (new_sensors) {
            uint32_t i = 31 - __builtin_clz(new_sensors);
            new_sensors &= ~(1<<i);
            dev->sensors[i].time = t;
        }
    }
}

static int data__poll(struct sensors_data_context_t *dev, sensors_data_t* values)
{ 
	struct input_event event;

    int acc_fd = dev->events_fd[0];
    int ori_fd = dev->events_fd[1];
    //int ls_fd = dev->events_fd[2];
    int got_syn = 0;
    int exit = 0;
    int nread;
    fd_set rfds;
    int n;

    if (acc_fd < 0) {
        LOGE("invalid compass file descriptor, fd=%d", acc_fd);
        return -1;
    }

    if (ori_fd < 0) {
        LOGE("invalid proximity-sensor file descriptor, fd=%d", ori_fd);
        return -1;
    }

    /*if (ls_fd < 0) {
        LOGE("invalid light-sensor file descriptor, fd=%d", ls_fd);
        return -1;
    }*/

    // there are pending sensors, returns them now...
    if (dev->pendingSensors) {
        LOGV("pending sensors 0x%08x", dev->pendingSensors);
        return pick_sensor(dev, values);
    }

    // wait until we get a complete event for an enabled sensor
    uint32_t new_sensors = 0;

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(acc_fd, &rfds);
        FD_SET(ori_fd, &rfds);
        //FD_SET(ls_fd, &rfds);

		/*n = select(__MAX(acc_fd, __MAX(ori_fd, ls_fd)) + 1, &rfds, NULL, NULL, NULL);*/

		n = select( __MAX(ori_fd, acc_fd) + 1, &rfds,
				NULL, NULL, NULL);

        if (n < 0) {
            LOGE("%s: error from select(%d, %d): %s",
                 __FUNCTION__,
                 acc_fd, ori_fd, strerror(errno));
            return -1;
        }
		
        if(delay_time_ms != 0)
            usleep(delay_time_ms * 200);
		
        if (FD_ISSET(acc_fd, &rfds)) {
            nread = read(acc_fd, &event, sizeof(event));
            if (nread == sizeof(event)) {
                new_sensors |= data__poll_process_acc_abs(dev, acc_fd, &event);
                LOGV("acc abs %08x", new_sensors);
                got_syn = event.type == EV_SYN;
                exit = got_syn && event.code == SYN_CONFIG;
                if (got_syn) {
                    LOGV("acc syn %08x", new_sensors);
                    data__poll_process_syn(dev, &event, SENSORS_ACCELERATION);
                    new_sensors = 0;
                }
            }
            else LOGE("acc read too small %d", nread);
        }
        else LOGV("acc fd is not set");

        if (FD_ISSET(ori_fd, &rfds)) {
            nread = read(ori_fd, &mrawData, sizeof(mrawData));
            if (nread == sizeof(mrawData)) {
			usleep(1000000);
#if 0
				 LOGE("mSensor1 data:  x,y,z:  %d, %d, %d\n", 
				 	 mrawData.x, 
				 	 mrawData.y, 
				 	 mrawData.z); 
#endif

				g_compass_data.x = mrawData.x;
				g_compass_data.y = mrawData.y;
				g_compass_data.z = mrawData.z;

                new_sensors |= data__poll_process_ori_abs(dev, ori_fd);
                LOGV("ori abs %08x", new_sensors);
                //got_syn |= event.type == EV_SYN;
                //exit |= got_syn && event.code == SYN_CONFIG;
				got_syn |= 1;
                if (got_syn) {
                    LOGV("ori syn %08x", new_sensors);
		    data__poll_process_syn_not_inputdev(dev, SENSORS_ORIENTATION|SENSORS_MAGNETIC_FIELD);
                    new_sensors = 0;
                }
            }
            else LOGV("ori read too small %d", nread);
        }
        else LOGV("ori fd is not set");

#if 0
        if (FD_ISSET(ls_fd, &rfds)) {
            nread = read(ls_fd, &event, sizeof(event));
            if (nread == sizeof(event)) {
                new_sensors |= data__poll_process_ls_abs(dev, ls_fd, &event);
                LOGV("ls abs %08x", new_sensors);
                got_syn |= event.type == EV_SYN;
                exit |= got_syn && event.code == SYN_CONFIG;
                if (got_syn) {
                    LOGV("ls syn %08x", new_sensors);
                    data__poll_process_syn(dev, &event, SENSORS_LIGHT);
                    new_sensors = 0;
                }
            }
            else 
                LOGV("ls read too small %d", nread);  
        }
        else 
            LOGV("ls fd is not set");
#endif

        if (exit) {
            // we use SYN_CONFIG to signal that we need to exit the
            // main loop.
            //LOGV("got empty message: value=%d", event->value);
	    LOGE("exit");
	    dev->pendingSensors = 0;
	    return 0x7FFFFFFF;
        }

        if (got_syn && dev->pendingSensors) {
            LOGV("got syn, picking sensor");
            /* LOGE("sensor is:%d [%f, %f, %f]", */
            /*         values->sensor, */
            /*         values->vector.x, */
            /*         values->vector.y, */
			/* 	 values->vector.z */
			/* 	 ); */
            return pick_sensor(dev, values);
        }
    }
}

/*****************************************************************************/

static int control__close(struct hw_device_t *dev)
{
    struct sensors_control_context_t* ctx =
        (struct sensors_control_context_t*)dev;

    if (ctx) {
        close_acc(ctx);
        close_ori(ctx);
        //close_ls(ctx);
        free(ctx);
    }
    return 0;
}

static int data__close(struct hw_device_t *dev)
{
    struct sensors_data_context_t* ctx = (struct sensors_data_context_t*)dev;

    if (ctx) {
        data__data_close(ctx);
        free(ctx);
    }
    return 0;
}


/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;

    if (!strcmp(name, SENSORS_HARDWARE_CONTROL)) {
        struct sensors_control_context_t *dev;
        dev = malloc(sizeof(*dev));
        memset(dev, 0, sizeof(*dev));
        dev->accd_fd = -1;
        dev->orid_fd = -1;
        //dev->lsd_fd = -1;
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = module;
        dev->device.common.close = control__close;
        dev->device.open_data_source = control__open_data_source;
        dev->device.activate = control__activate;
        dev->device.set_delay= control__set_delay;
        dev->device.wake = control__wake;
        *device = &dev->device.common;
    } else if (!strcmp(name, SENSORS_HARDWARE_DATA)) {
        struct sensors_data_context_t *dev;
        dev = malloc(sizeof(*dev));
        memset(dev, 0, sizeof(*dev));
        dev->events_fd[0] = -1;
        dev->events_fd[1] = -1;
        //dev->events_fd[2] = -1;
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = module;
        dev->device.common.close = data__close;
        dev->device.data_open = data__data_open;
        dev->device.data_close = data__data_close;
        dev->device.poll = data__poll;
        *device = &dev->device.common;
    }
    return status;
}
