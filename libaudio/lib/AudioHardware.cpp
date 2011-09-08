/*
**
** Copyright 2007, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include <stdint.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>

#define LOG_TAG "AudioHardwareASTER"
#include <utils/Log.h>
#include <utils/String8.h>
#include "AudioHardware.h"

#define MMAP_ENABLE

namespace android {

// ----------------------------------------------------------------------------
static char const * const audioDevice[] =
{
    "default", //ALSA_STEREO_OUT
//  "hw:0,1",
    "plughw:0,0",//ALSA_MONO_OUT
//  "default",
    "default", //ALSA_MONO_IN
};

static uint32_t supportedOutSampleRate[] = 
{
    8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int supportedOutChannels[] = 
{
    1, 2
};

static int supportedOutFormat[] = 
{
    AudioSystem::PCM_16_BIT
};

static uint32_t supportedInSampleRate[] = 
{
    8000, 16000, 32000, 44100, 48000
};

static int supportedInChannels[] = 
{
    1, 2
};

static int supportedInFormat[] = 
{
    AudioSystem::PCM_16_BIT
};

static bool isOutSampleRateSupported(uint32_t rate)
{
    int i;
    int len = sizeof(supportedOutSampleRate) / sizeof(uint32_t);

    for(i = 0; i < len; i++)
    {
        if (rate == supportedOutSampleRate[i])
        {
            return true;
        }
    }

    return false;    
}

static bool isOutChannelsSupported(int channels)
{
    int i;
    int len = sizeof(supportedOutChannels) / sizeof(int);

    for(i = 0; i < len; i++)
    {
        if (channels == supportedOutChannels[i])
        {
            return true;
        }
    }

    return false;    
}

static bool isOutFormatSupported(int format)
{
    int i;
    int len = sizeof(supportedOutFormat) / sizeof(int);

    for(i = 0; i < len; i++)
    {
        if (format == supportedOutFormat[i])
        {
            return true;
        }
    }

    return false;    
}

static bool isInSampleRateSupported(uint32_t rate)
{
    int i;
    int len = sizeof(supportedInSampleRate) / sizeof(uint32_t);

    for(i = 0; i < len; i++)
    {
        if (rate == supportedInSampleRate[i])
        {
            return true;
        }
    }

    return false;    
}

static bool isInChannelsSupported(int channels)
{
    int i;
    int len = sizeof(supportedInChannels) / sizeof(int);

    for(i = 0; i < len; i++)
    {
        if (channels == supportedInChannels[i])
        {
            return true;
        }
    }

    return false;    
}

static bool isInFormatSupported(int format)
{
    int i;
    int len = sizeof(supportedInFormat) / sizeof(int);

    for(i = 0; i < len; i++)
    {
        if (format == supportedInFormat[i])
        {
            return true;
        }
    }

    return false;
}

// ----------------------------------------------------------------------------
/*
 Class ALSAHandle implements ALSA play/record audio data API for AudioStreamIn and AudioStreamOut
*/
ALSAHandle::ALSAHandle()
{
    mDeviceType = ALSA_NULL;
    mPcmHandle = NULL;
    mStreamType = SND_PCM_STREAM_PLAYBACK;

#ifdef MMAP_ENABLE
    writei_func = snd_pcm_mmap_writei;
    readi_func = snd_pcm_mmap_readi;
#else
    writei_func = snd_pcm_writei;
    readi_func = snd_pcm_readi;
#endif

#ifdef DUMP_PCM
    mPcmFile = NULL;
#endif
}

ALSAHandle::~ALSAHandle()
{
    close();
}

status_t ALSAHandle::open(audioDeviceType type)
{
    int err = -1;

    LOGI("ALSAHandle: open %d", type);

    if (mDeviceType != ALSA_NULL)
    {
        LOGE("ALSAHandle: Device %d is opening, close %d before open %d", mDeviceType, mDeviceType, type);
        return -1;
    }

    switch (type)
    {
        case ALSA_STEREO_OUT:
        case ALSA_MONO_OUT:
            {
                mStreamType = SND_PCM_STREAM_PLAYBACK;
            }
            break;

        case ALSA_MONO_IN:
            {
                mStreamType = SND_PCM_STREAM_CAPTURE;
            }
            break;

        default:
            {
                LOGE("ALSAHandle: Invalid audioDeviceType: %d", type);
                mPcmHandle = NULL;
                return -1;
            }
            break;
    }

    LOGI("ALSAHandle: snd_pcm_open ALSA device %s streamtype %d", audioDevice[type], (int)mStreamType);

    err = snd_pcm_open(&mPcmHandle, audioDevice[type], mStreamType, 0);
    if ((err < 0) || (mPcmHandle == NULL))
    {
        LOGE("ALSAHandle: alsa open error: %s", snd_strerror(err));
        mPcmHandle = NULL;
        return -1;
    }

    mDeviceType = type;
 
#ifdef DUMP_PCM
    const char *filename = "/sdcard/alsa.pcm";
    mPcmFile = fopen(filename, "wb");
#endif

    return NO_ERROR;
}

status_t ALSAHandle::setHwParams(snd_pcm_format_t format, unsigned int channels, unsigned int sampleRate, snd_pcm_uframes_t periodSize)
{
    int err = -1;
    snd_pcm_hw_params_t *params = NULL;

    if (NULL == mPcmHandle)
    {
        LOGE("ALSAHandle: setHwParams mPcmHandle is NULL");
        return -1;
    }
    
    mHwparams.format = format;  //format is always SND_PCM_FORMAT_S16_LE
    mHwparams.channels = channels;
    mHwparams.rate = sampleRate;
    mHwparams.periodSize = periodSize;
    mHwparams.bits_per_sample = snd_pcm_format_physical_width(mHwparams.format);
    mHwparams.bits_per_frame = mHwparams.bits_per_sample * mHwparams.channels;
    mHwparams.bytes_per_frame = mHwparams.bits_per_frame / 8;
    
    snd_pcm_hw_params_alloca(&params);

    err = snd_pcm_hw_params_any(mPcmHandle, params);
    if (err < 0)
    {
        LOGE("ALSAHandle: Broken configuration for this PCM: no configurations available, result is %s",
                                                                                        snd_strerror(err));
        return -1;
    }

    #ifdef MMAP_ENABLE
    snd_pcm_access_mask_t *mask = (snd_pcm_access_mask_t *)alloca(snd_pcm_access_mask_sizeof());
    snd_pcm_access_mask_none(mask);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
    err = snd_pcm_hw_params_set_access_mask(mPcmHandle, params, mask);
    #else
    err = snd_pcm_hw_params_set_access(mPcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    #endif
    
    if (err < 0)
    {
        LOGE("ALSAHandle: Access type not available, result is %s", snd_strerror(err));
        return -1;
    }
    
    err = snd_pcm_hw_params_set_format(mPcmHandle, params, mHwparams.format);
    if (err < 0)
    {
        LOGE("ALSAHandle: Sample format non available, result is %s", snd_strerror(err));
        return -1;
    }
    
    err = snd_pcm_hw_params_set_channels(mPcmHandle, params, mHwparams.channels);
    if (err < 0)
    {
        LOGE("ALSAHandle: Channels count non available, result is %s", snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params_set_rate_near(mPcmHandle, params, &(mHwparams.rate), 0);
    if (err<0)
    {
        LOGE("ALSAHandle: Set rate error! result is %s", snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params_set_period_size_near(mPcmHandle, params, &(mHwparams.periodSize), 0);
    if (err<0)
    {
        LOGE("ALSAHandle: Set periold size error! result is %s", snd_strerror(err));
        return -1;
    }
    
    mHwparams.bufferSize = mHwparams.periodSize*4;
    err = snd_pcm_hw_params_set_buffer_size_near(mPcmHandle, params, &(mHwparams.bufferSize));
    if (err<0)
    {
        LOGE("ALSAHandle: Set buffer size error! result is %s", snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params(mPcmHandle, params);
    if (err < 0)
    {
        LOGE("ALSAHandle: Unable to install hw params, result is %s", snd_strerror(err));
        return -1;
    }

    snd_pcm_hw_params_get_period_size(params, &(mHwparams.periodSize), 0);
    snd_pcm_hw_params_get_buffer_size(params, &(mHwparams.bufferSize));

    LOGI("ALSAHandle: periodSize: %d, bufferSize: %d", (int)(mHwparams.periodSize), (int)(mHwparams.bufferSize));

    return NO_ERROR;
}

status_t ALSAHandle::setSwParams(ALSA_SET_SW_FLAG flag)
{
    snd_pcm_sw_params_t * softwareParams = NULL;
    int err = -1;

    if (NULL == mPcmHandle)
    {
        LOGE("ALSAHandle: setSwParams mPcmHandle is NULL");
        return -1;
    }

    snd_pcm_uframes_t bufferSize = 0;
    snd_pcm_uframes_t periodSize = 0;
    snd_pcm_uframes_t startThreshold = 0, stopThreshold = 0;

    if (snd_pcm_sw_params_malloc(&softwareParams) < 0)
    {
        LOGE("ALSAHandle: Failed to allocate ALSA software parameters!");
        return NO_INIT;
    }

    // Get the current software parameters
    err = snd_pcm_sw_params_current(mPcmHandle, softwareParams);
    if (err < 0)
    {
        LOGE("ALSAHandle: Unable to get software parameters: %s", snd_strerror(err));
        goto done;
    }

    // Configure ALSA to start the transfer when the buffer is almost full.
    snd_pcm_get_params(mPcmHandle, &bufferSize, &periodSize);

    // For playback, configure ALSA to start the transfer when the buffer is full.
    // For recording, configure ALSA to start the transfer on the first frame.
    if (flag == SW_PLAY)
    {
        startThreshold = bufferSize - periodSize;
        stopThreshold = bufferSize;
    }
    else if (flag == SW_RECORD)
    {
        startThreshold = 1;
        stopThreshold = bufferSize;
    }

    err = snd_pcm_sw_params_set_start_threshold(mPcmHandle, softwareParams, startThreshold);
    if (err < 0)
    {
        LOGE("ALSAHandle: Unable to set start threshold to %lu frames: %s", startThreshold, snd_strerror(err));
        goto done;
    }

    err = snd_pcm_sw_params_set_stop_threshold(mPcmHandle, softwareParams, stopThreshold);
    if (err < 0)
    {
        LOGE("ALSAHandle: Unable to set stop threshold to %lu frames: %s", stopThreshold, snd_strerror(err));
        goto done;
    }

    // Allow the transfer to start when at least periodSize samples can be processed.
    err = snd_pcm_sw_params_set_avail_min(mPcmHandle, softwareParams, periodSize);
    if (err < 0)
    {
        LOGE("ALSAHandle: Unable to configure available minimum to %lu: %s", periodSize, snd_strerror(err));
        goto done;
    }

    // Commit the software parameters back to the device.
    err = snd_pcm_sw_params(mPcmHandle, softwareParams);
    if (err < 0)
    {
        LOGE("ALSAHandle: Unable to configure software parameters: %s", snd_strerror(err));
    }

done:
    snd_pcm_sw_params_free(softwareParams);

    LOGI("ALSAHandle: flag = %d, err = %d", flag, err);

    return err;
}

void ALSAHandle::close()
{
    LOGI("ALSAHandle: device %d", mDeviceType);

    mDeviceType = ALSA_NULL;
    
    if (NULL != mPcmHandle) 
    {
        snd_pcm_close(mPcmHandle);
        mPcmHandle = NULL;
    }
    
#ifdef DUMP_PCM
    if (mPcmFile) 
    {
        fflush(mPcmFile);
        fclose(mPcmFile);
        mPcmFile = NULL;
    }
#endif
}

ssize_t ALSAHandle::write(const void* buffer, size_t bytes)
{
    ssize_t r = 0, remain_frames = 0, written_frames = 0;
    char * data = (char*)buffer;

    if (NULL == mPcmHandle)
    {
        LOGE("ALSAHandle: mPcmHandle is NULL");
        return -1;
    }

    remain_frames = bytes / mHwparams.bytes_per_frame;

#ifdef DUMP_PCM
    if (mPcmFile) 
    {
        fwrite(buffer, sizeof(char), bytes, mPcmFile);
        fflush(mPcmFile);
    }
#endif

    while (remain_frames > 0)
    {
        r = writei_func(mPcmHandle, data, remain_frames);
        if (r == -EAGAIN || (r >= 0 && r < remain_frames)) 
        {
            LOGE("ALSAHandle: write error = -EAGAIN, r = %d, bytes = %d, periodSize = %d, remain_frames= %d",
                                          (int)r, (int)bytes, (int)mHwparams.periodSize, (int)remain_frames);
            snd_pcm_wait(mPcmHandle, 1000);
        }
        else if (r == -EPIPE)
        {
            LOGE("ALSAHandle: write error: r = -EPIPE");
            xrun();
        }
        else if (r == -ESTRPIPE)
        {
            LOGE("ALSAHandle: write error: r = -ESTRPIPE");
            suspend();
        } 
        else if (r < 0)
        {
            LOGE("ALSAHandle: write error: r = %d", (int)r);
            return r;
        }

        if (r > 0)
        {
            written_frames += r;
            remain_frames  -= r;
            data += r * mHwparams.bytes_per_frame;
        }
    }

    return written_frames * mHwparams.bytes_per_frame;
}

ssize_t ALSAHandle::read(void* buffer, ssize_t bytes)
{
    ssize_t n = 0, remain_frames = 0, read_frames = 0;
    char *data = (char*)buffer;

    if (NULL == mPcmHandle)
    {
        LOGE("ALSAHandle: mPcmHandle is NULL");
        return -1;
    }

    remain_frames = bytes / mHwparams.bytes_per_frame;
    while (remain_frames > 0)
    {
        n = readi_func(mPcmHandle, data, remain_frames);

        if (n == -EAGAIN || (n >= 0 && n < remain_frames)) 
        {
            LOGE("ALSAHandle: read error = -EAGAIN, n = %d, bytes = %d, periodSize = %d, remain_frames= %d",
                                         (int)n, (int)bytes, (int)mHwparams.periodSize, (int)remain_frames); 
            snd_pcm_wait(mPcmHandle, 1000);
        } 
        else if (n == -EPIPE) 
        {
            LOGE("ALSAHandle: read error: r = -EPIPE");
            xrun();
        } 
        else if (n == -ESTRPIPE)
        {
            LOGE("ALSAHandle: read error: r = -ESTRPIPE");
            suspend();
        } 
        else if (n < 0) 
        {
            LOGE("ALSAHandle: read error: %s", snd_strerror(n));
            return n;
        }
        
        if (n > 0)
        {
            read_frames += n;
            remain_frames -= n;
            data += n* mHwparams.bytes_per_frame;
        }
    };

#ifdef DUMP_PCM
    if (mPcmFile) 
    {
        fwrite(buffer, sizeof(char), bytes, mPcmFile);
        fflush(mPcmFile);
    }
#endif

    return read_frames * mHwparams.bytes_per_frame;
}

ALSAHandle::audioDeviceType ALSAHandle::status()
{
    return mDeviceType;
}

ssize_t ALSAHandle::xrun(void)
{
#ifndef timersub
#define    timersub(a, b, result) \
do { \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
    if ((result)->tv_usec < 0) { \
        --(result)->tv_sec; \
        (result)->tv_usec += 1000000; \
    } \
} while (0)
#endif

    snd_pcm_status_t *status = NULL;
    int res = -1;
    
    snd_pcm_status_alloca(&status);
   
    if ((res = snd_pcm_status(mPcmHandle, status)) < 0) 
    {
        LOGE("ALSAHandle: status error: %s", snd_strerror(res));
        return -1;
    }
    
    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN)
    {
        struct timeval now, diff, tstamp;
        gettimeofday(&now, 0);
        snd_pcm_status_get_trigger_tstamp(status, &tstamp);
        timersub(&now, &tstamp, &diff);

        LOGI("ALSAHandle: %s!!! (at least %.3f ms long)", mStreamType == SND_PCM_STREAM_PLAYBACK ? "underrun" : "overrun",diff.tv_sec * 1000 + diff.tv_usec / 1000.0);

        if ((res = snd_pcm_prepare(mPcmHandle)) < 0)
        {
            LOGE("ALSAHandle: prepare error: %s", snd_strerror(res));
            return -1;
        }

        return 0;        /* ok, data should be accepted again */
    } 
    
    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING)
    {
        if (mStreamType == SND_PCM_STREAM_CAPTURE)
        {
            LOGE("ALSAHandle: capture stream format change? attempting recover...");

            if ((res = snd_pcm_prepare(mPcmHandle)) < 0) 
            {
                LOGE("ALSAHandle: prepare error: %s", snd_strerror(res));
                return -1;
            }

            return 0;
        }
    }

    LOGE("ALSAHandle: read/write error, state = %s", snd_pcm_state_name(snd_pcm_status_get_state(status)));

    return -1;
}

ssize_t ALSAHandle::suspend(void)
{
    int res = -1;

    while ((res = snd_pcm_resume(mPcmHandle)) == -EAGAIN)
    {
        sleep(1);   /* wait until suspend flag is released */
    }

    if (res < 0)
    {
        if ((res = snd_pcm_prepare(mPcmHandle)) < 0)
        {
            LOGE("ALSAHandle: prepare error: %s", snd_strerror(res));
            return -1;
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
AudioHardware::AudioHardware()
{
    mOutput = NULL;
    mInput = NULL;
    mAlsaHandle = new ALSAHandle();
    mCurMode = mMode;
    mCurDevices = 0;
    mVoiceVolume = 100;
    mMicMute = false;
    mFirstEnableDevice = false;

    HWA_Init();
}

AudioHardware::~AudioHardware()
{
    if (NULL!=mOutput)
    {
        delete mOutput;
    }
    if (NULL!=mInput) 
    {
        delete mInput;
    }
    if (NULL!=mAlsaHandle) 
    {
        delete mAlsaHandle;
    }
}

status_t AudioHardware::initCheck()
{
    return NO_ERROR;
}

size_t AudioHardware::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{
    if (format != AudioSystem::PCM_16_BIT) {
            LOGW("getInputBufferSize bad format: %d", format);
            return 0;
    }
    if (channelCount < 1 || channelCount > 2) {
            LOGW("getInputBufferSize bad channel count: %d", channelCount);
            return 0;
    }
    return 320;
}
AudioStreamOut* AudioHardware::openOutputStream(
                                uint32_t devices,
                                int *format,
                                uint32_t *channels,
                                uint32_t *sampleRate,
                                status_t *status) 
{
    AutoMutex lock(mLock);

    LOGI("openOutputStream: devices: 0x%x format: %d, channels: 0x%x, sampleRate: %d",
                                              devices, *format, *channels, *sampleRate);

    // only one output stream allowed
    if (mOutput)
    {
        if (status)
        {
            *status = INVALID_OPERATION;
        }

        return NULL;
    }

    // create new output stream
    AudioStreamOutASTER* out = new AudioStreamOutASTER();
    status_t lStatus = out->set(this,devices, format, channels, sampleRate);
    if (status)
    {
        *status = lStatus;
    }

    if (lStatus == NO_ERROR)
    {
        mOutput = out;
    }
    else
    {
        delete out;
    }

    return mOutput;
}

void AudioHardware::closeOutputStream(AudioStreamOut* out)
{
    LOGI("closeOutputStream: closing Output Stream");

    if (out == mOutput)
    {
        mOutput = NULL;
    }

    if (out) 
    {
        delete out;
    }
}

AudioStreamIn* AudioHardware::openInputStream(
        uint32_t devices,
        int *format,
        uint32_t *channels,
        uint32_t *sampleRate,
        status_t *status,
        AudioSystem::audio_in_acoustics acoustics)
{
    AutoMutex lock(mLock);
    
    LOGI("openInputStream: devices: 0x%x, format: %d, channels: %d, sampleRate: %d",
                                          devices, *format, *channels, *sampleRate);

    // only one input stream allowed
    if (mInput) 
    {
        if (status) 
        {
            *status = INVALID_OPERATION;
        }

        return NULL;
    }

    // create new output stream
    AudioStreamInASTER* in = new AudioStreamInASTER();
    status_t lStatus = in->set(this,devices, format, channels, sampleRate, acoustics);
    if (status) 
    {
        *status = lStatus;
    }

    if (lStatus == NO_ERROR) 
    {
        mInput = in;
    } 
    else 
    {
        delete in;
    }

    return mInput;
}

void AudioHardware::closeInputStream(AudioStreamIn* in)
{
    LOGI("closeInputStream: closing Input Stream");

    if (in == mInput)
    {
        mInput = NULL;
    }

    if (in) 
    {
        delete in;
    }
}

status_t AudioHardware::setMode(int mode)
{
    status_t status = AudioHardwareBase::setMode(mode);
    return status;
}

status_t AudioHardware::setVoiceVolume(float v)
{
    AutoMutex lock(mLock);

    int CurrentMode = getCurMode();

    if (v < 0 || v > 1)
    {
        v = 1;
    }

    mVoiceVolume = (unsigned int)(v * 100);

    LOGI("setVoiceVolume: volume: %u, current mode: %d", mVoiceVolume, CurrentMode);

    return NO_ERROR;
}

status_t AudioHardware::setMasterVolume(float v)
{
    // Implement: set master volume
    // return error - software mixer will handle it
    return INVALID_OPERATION;
}

status_t AudioHardware::setMicMute(bool state)
{
    AutoMutex lock(mLock);    
    int CurrentMode = 0;

    mMicMute = state;

    CurrentMode = getCurMode();

    if (CurrentMode != AudioSystem::MODE_IN_CALL)
    {
        return NO_ERROR;
    }

    return NO_ERROR;
}

status_t AudioHardware::getMicMute(bool* state)
{
    *state = mMicMute;
    return NO_ERROR;
}

status_t AudioHardware::setParameters(const String8& keyValuePairs)
{
    AudioParameter param = AudioParameter(keyValuePairs);
    status_t status = NO_ERROR;
    String8 values;
    const  char *value_t;

    if (keyValuePairs.length() == 0) 
        return BAD_VALUE;

    LOGI("setParameters: %s", keyValuePairs.string());

    return status;
}

String8 AudioHardware::getParameters(const String8& keys)
{
    AudioParameter param = AudioParameter(keys);

    LOGI("getParameters: %s", param.toString().string());
    return param.toString();
}

status_t AudioHardware::setModeAndDevices(int on, int mode, uint32_t devices)
{
    LOGI("setModeAndDevices: on: %d, mode: %d, devices: 0x%x",on, mode, devices);

    if(0x0 == devices)
    {
        goto end;
    }

    switch(mode)
    {
        case AudioSystem::MODE_NORMAL:
        case AudioSystem::MODE_RINGTONE:
        {
            if(devices & AudioSystem::DEVICE_IN_BUILTIN_MIC || devices & AudioSystem::DEVICE_IN_AMBIENT)
            {
                if(on)
                {
			HWA_AudioDeviceEnable(HWA_LOUD_MIC, HWA_I2S, 100);
                }
                else
                {
			HWA_AudioDeviceDisable(HWA_LOUD_MIC, HWA_I2S);
                }
            }
           
            if (devices & AudioSystem::DEVICE_OUT_EARPIECE) 
            {
                if(on)
                {
			HWA_AudioDeviceEnable(HWA_LOUDSPEAKER, HWA_I2S, 100);
                }
                else
                {
			HWA_AudioDeviceDisable(HWA_LOUDSPEAKER, HWA_I2S);
                }
            }

            if(devices & AudioSystem::DEVICE_OUT_SPEAKER)
            {
                if(on)
                {
			HWA_AudioDeviceEnable(HWA_LOUDSPEAKER, HWA_I2S, 100);
                }
                else
                {
			HWA_AudioDeviceDisable(HWA_LOUDSPEAKER, HWA_I2S);
                }
            }

            if(devices & AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET)
            {
                if(on)
                {
                }
                else
                {
                }
            }
                        
            if(devices & AudioSystem::DEVICE_IN_WIRED_HEADSET)
            {
                if(on)
                {
			HWA_AudioDeviceEnable(HWA_HP_MIC, HWA_I2S, 100);
                }
                else
                {
			HWA_AudioDeviceDisable(HWA_HP_MIC, HWA_I2S);
                }
            }

            if(devices & (AudioSystem::DEVICE_OUT_WIRED_HEADSET | AudioSystem::DEVICE_OUT_WIRED_HEADPHONE))
            {
                if(on)
                {
			HWA_AudioDeviceEnable(HWA_HP_SPEAKER, HWA_I2S, 100);
                }
                else
                {
			HWA_AudioDeviceDisable(HWA_HP_SPEAKER, HWA_I2S);
                }
            }

        }            
        break;
        //reserve for PAD call feature 
        case AudioSystem::MODE_IN_CALL:
        {
            if(devices & AudioSystem::DEVICE_OUT_EARPIECE)
            {
                if(on) 
                {
                }
                else 
                {
                }
            }

            if(devices & AudioSystem::DEVICE_OUT_SPEAKER)
            {
                if(on)
                {
                }
                else 
                {
                }
            }

            if(devices & (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                          AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                          AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT))
            {
                if(on) 
                {
                }
                else 
                {
                }
            }

            if(devices & AudioSystem::DEVICE_OUT_WIRED_HEADSET)
            {
                if(on)
                {   
                }
                else
                {
                }
            }
            
            if(devices & AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)
            {
                if(on)
                {
                }
                else
                {
                }
            }
        }
        break;
    }

end:
    return NO_ERROR;
}

status_t AudioHardware::updateAudioDevices(AudioStreamInASTER* input)
{
    int doMode = mMode;
    uint32_t doDevices = 0x0;

    doDevices = mOutput->devices();

    if (0x0 == mCurDevices && false == mFirstEnableDevice)
    {
	mFirstEnableDevice = true;
    	mCurDevices = doDevices;
	return NO_ERROR;
    }


    if(input)
    {
        doDevices |= input->devices();
    }

    LOGI("updateAudioDevices: Current Mode: %d, Current Device: 0x%x, new Mode: %d, new Device: 0x%x",
                                                            mCurMode, mCurDevices, doMode, doDevices);

    if ((mCurMode != AudioSystem::MODE_IN_CALL) && (doMode == AudioSystem::MODE_IN_CALL))
    {
        mMicMute = false; // MIC is unmute at the beginning of voice call
    }
    else if ((mCurMode == AudioSystem::MODE_IN_CALL) && (doMode != AudioSystem::MODE_IN_CALL))
    {
        mMicMute = false; // MIC is unmute at the end of voice call
    }

    /*2010-11-19-Nathan:don't enable the same devices when voice call recording.*/
    if (mCurMode == AudioSystem::MODE_IN_CALL && 
        doMode == AudioSystem::MODE_IN_CALL &&
        doDevices & mCurDevices)
    {
        LOGI("updateAudioDevices: Don't enable the same devices when voice call recording");
        mCurDevices = doDevices;
        return NO_ERROR;
    }

    //Disable current audio routing
    switch(mCurMode)
    {
        case AudioSystem::MODE_NORMAL:
        case AudioSystem::MODE_RINGTONE:
            {
                setModeAndDevices(0, mCurMode, mCurDevices);
            }
            break;

        case AudioSystem::MODE_IN_CALL:
            {
                setModeAndDevices(0, mCurMode, mCurDevices);
            }
            break;

        default:
            break;
    }

    //Enable audio routing
    switch(doMode)
    {
        case AudioSystem::MODE_NORMAL:
        case AudioSystem::MODE_RINGTONE:
            {
                setModeAndDevices(1, doMode, doDevices);
            }
            break;

        case AudioSystem::MODE_IN_CALL:
            {
                setModeAndDevices(1, doMode, doDevices);
            }
            break;

        default:
            break;
    }

    mCurMode = doMode;
    mCurDevices = doDevices;

    LOGI("updateAudioDevices: return");

    return NO_ERROR;
}

status_t AudioHardware::dump(int fd, const Vector<String16>& args) 
{ 
    if (mInput) 
    { 
        mInput->dump(fd, args); 
    }

    if (mOutput) 
    { 
        mOutput->dump(fd, args); 
    } 

    return NO_ERROR; 
} 

int AudioHardware::getCurMode(void)
{
    return mCurMode;
}

uint32_t AudioHardware::getCurDevices(void)
{
    return mCurDevices;
}

AudioStreamInASTER* AudioHardware::getInputStream(void)
{
    return mInput;
}

// ----------------------------------------------------------------------------
AudioStreamOutASTER::AudioStreamOutASTER()
{
    mSampleRate = 44100;
    mBufferSize = 6144;
    mChannels = AudioSystem::CHANNEL_OUT_STEREO;
    mChannelCounts = 2;
    mAudioHardware = NULL;
    mAlsaHandle = new ALSAHandle();
    mDevices = 0;
    mFrameCount = 0;
}

AudioStreamOutASTER::~AudioStreamOutASTER()
{
    standby();

    if (mAlsaHandle)
    {
        delete mAlsaHandle;
        mAlsaHandle = NULL;
    }
}

status_t AudioStreamOutASTER::setParameters(const String8& keyValuePairs)
{
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 key = String8(AudioParameter::keyRouting);
    status_t status = NO_ERROR;
    int device;

    LOGI("AudioStreamOutASTER: setParameters: %s", keyValuePairs.string());

    if (param.getInt(key, device) == NO_ERROR)
    {
        if(device == 0)
        {
            LOGW("AudioStreamOutASTER: get device is :0x%x", device);
            return status;
        }
		
        mDevices = device;
        LOGI("AudioStreamOutASTER: set AudioStreamOut Device 0x%x", mDevices);
        LOGI_IF( mAudioHardware->getInputStream(), "AudioStreamOutASTER: Has input streaming, will merge the input devices to output devices");
        status = mAudioHardware->getInputStream() ? mAudioHardware->updateAudioDevices(mAudioHardware->getInputStream()) : mAudioHardware->updateAudioDevices(NULL);
	    param.remove(key);
    }

    if (param.size()) 
    {
        status = BAD_VALUE;
    }

    return status;
}

String8 AudioStreamOutASTER::getParameters(const String8& keys)
{
    AudioParameter param = AudioParameter(keys);
    String8 value;
    String8 key = String8(AudioParameter::keyRouting);

    if (param.get(key, value) == NO_ERROR)
    {
        LOGI("AudioStreamOutASTER: get AudioStreamOut devices %x", mDevices);
        param.addInt(key, (int)mDevices);
    }

    LOGI("AudioStreamOutASTER: getParameters: %s", param.toString().string());

    return param.toString();
}

status_t AudioStreamOutASTER::set(
            AudioHardware *hw,
            uint32_t devices,
            int *pFormat,
            uint32_t *pChannels,
            uint32_t *pRate)
{
    LOGI("AudioStreamOutASTER: set() devices = 0x%x , format = %d, channels = 0x%x , rate = %d", 
                                                     devices, *pFormat, (int)*pChannels, *pRate);
    
    int lFormat = pFormat ? *pFormat : 0;
    uint32_t lChannels = pChannels ? *pChannels : 0;
    uint32_t lRate = pRate ? *pRate : 0;

    // fix up defaults
    if (lFormat == 0) 
    {
        lFormat = format();
    }

    if (lChannels == 0) 
    {
        lChannels = channels();
    }

    if (lRate == 0) 
    {
        lRate = sampleRate();
    }

    // check values
    if ((lFormat != format()) || (lChannels != channels()) || (lRate != sampleRate())) 
    {
        if (pFormat)
        {
            *pFormat = format();
        }

        if (pChannels) 
        {
            *pChannels = channels();
        }

        if (pRate) 
        {
            *pRate = sampleRate();
        }

        return BAD_VALUE;
    }

    if (pFormat)
    {
        *pFormat = lFormat;
    }

    if (pChannels) 
    {
        *pChannels = lChannels;
    }

    if (pRate) 
    {
        *pRate = lRate;
    }

    if(lChannels == AudioSystem::CHANNEL_OUT_STEREO)
    {
        mChannelCounts = 2;
    }
    else
    {
        mChannelCounts = 1;
    }
 
    LOGI("AudioStreamOutASTER: set() devices = 0x%x , format = %d, channels = 0x%x , rate = %d, channelcounts = %d",
                                                            (int)devices, lFormat, lChannels, lRate, mChannelCounts);

    mAudioHardware = hw;
    mDevices = devices;

    return NO_ERROR;
}

uint32_t  AudioStreamOutASTER::sampleRate() const
{
    return mSampleRate;
}

size_t AudioStreamOutASTER::bufferSize() const
{
    return mBufferSize;
}

uint32_t AudioStreamOutASTER::channels() const
{
    return mChannels;
}

ssize_t AudioStreamOutASTER::write(const void* buffer, size_t bytes)
{
    int         mode;

    if (mAudioHardware == NULL)
    {
        return bytes;
    }

    mode = mAudioHardware->getCurMode();

    if (mode == AudioSystem::MODE_IN_CALL)
    {
        usleep(bytes * 1000000 / sizeof(int16_t) / mChannelCounts / sampleRate());
        return bytes;
    }
    else if (devices() & AudioSystem::DEVICE_OUT_BLUETOOTH_A2DP){
        usleep(bytes * 1000000 / sizeof(int16_t) / mChannelCounts / sampleRate());
        return bytes;
    }
    else if (devices() & (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO|AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET|AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT))
    {
        usleep(bytes * 1000000 / sizeof(int16_t) / mChannelCounts / sampleRate());
        return bytes;
    }
    else
    {
        if (mAlsaHandle->status() != ALSAHandle::ALSA_STEREO_OUT 
            && mAlsaHandle->status() != ALSAHandle::ALSA_NULL)
        {
            this->standby();
        }

        if (mAlsaHandle->status() == ALSAHandle::ALSA_NULL)
        {
            mAlsaHandle->open(ALSAHandle::ALSA_STEREO_OUT);
            mAlsaHandle->setHwParams(SND_PCM_FORMAT_S16_LE, mChannelCounts, sampleRate(), (snd_pcm_uframes_t)this->periodSize());
            mAlsaHandle->setSwParams(ALSAHandle::SW_PLAY);
            mAudioHardware->setModeAndDevices(1, mode, devices());
	    usleep(200000);
        }

        if (mAlsaHandle->status() != ALSAHandle::ALSA_NULL)
        {
            mAlsaHandle->write(buffer, bytes);
            mFrameCount += bytes;
        }

        return bytes;
    } 
    return -1;
}

status_t AudioStreamOutASTER::standby()
{
    LOGD("AudioStreamOutASTER: standby");
    int mode = 0;
    
    if(!mAudioHardware->getInputStream())
    {
    	if (mAudioHardware)
    	{
        	mode = mAudioHardware->getCurMode();

        	if (mode != AudioSystem::MODE_IN_CALL)
        	{
            		mAudioHardware->setModeAndDevices(0, mode, devices());
        	}
    	}
    }

    if (mAlsaHandle)
    {
        mAlsaHandle->close();
    }
    
    mFrameCount = 0;

    return NO_ERROR;
}

// return the number of audio frames written by the audio dsp to DAC since
// the output has exited standby
status_t AudioStreamOutASTER::getRenderPosition(uint32_t *dspFrames)
{
    *dspFrames = mFrameCount;
    return NO_ERROR;
}

status_t AudioStreamOutASTER::dump(int fd, const Vector<String16>& args) 
{ 
    const size_t SIZE = 256; 
    char buffer[SIZE]; 
    String8 result; 
    snprintf(buffer, SIZE, "AudioStreamOutASTER::dump\n"); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tsample rate: %d\n", sampleRate()); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tbuffer size: %d\n", bufferSize()); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tchannel count: %d\n", mChannelCounts); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tformat: %d\n", format()); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tmAudioHardware: %p\n", mAudioHardware); 
    result.append(buffer); 
    ::write(fd, result.string(), result.size()); 
    return NO_ERROR; 
} 

// ----------------------------------------------------------------------------
// record functions
AudioStreamInASTER::AudioStreamInASTER()
{
    mAudioHardware = NULL;
    mAlsaHandle = new ALSAHandle();
    mDevices = 0;
    mInSampleRate = 8000;
    mFramesLost = 0;
}

AudioStreamInASTER::~AudioStreamInASTER()
{
    standby();

    if (mAlsaHandle)
    {
        delete mAlsaHandle;
        mAlsaHandle = NULL;
    }
}

status_t AudioStreamInASTER::set(
            AudioHardware *hw,
            uint32_t devices,
            int *pFormat,
            uint32_t *pChannels,
            uint32_t *pRate,
            AudioSystem::audio_in_acoustics acoustics)
{


    LOGI("AudioStreamInASTER: set() devices = 0x%x , format = %d, channels = 0x%x , rate = %d",
                                                 (int)devices,*pFormat,(int)*pChannels,*pRate );

    if (pFormat == 0 || *pFormat != AudioSystem::PCM_16_BIT) 
    {
        *pFormat = AudioSystem::PCM_16_BIT;
        return BAD_VALUE;
    }

    if (pRate == 0) 
    {
        return BAD_VALUE;
    }

    if(!isInSampleRateSupported(*pRate))
    {
        *pRate = 8000;
        return BAD_VALUE;
    }
    else
    {
        mInSampleRate = *pRate;
    }

    if (pChannels == 0 || (*pChannels != AudioSystem::CHANNEL_IN_MONO && *pChannels != AudioSystem::CHANNEL_IN_STEREO)) 
    {
        *pChannels = AudioSystem::CHANNEL_IN_MONO;
        return BAD_VALUE;
    }

    LOGI("AudioStreamInASTER: set(%d, %d, %u)", *pFormat, *pChannels, *pRate);
   
    if(*pChannels == AudioSystem::CHANNEL_IN_MONO)
    {
        mChannelCounts = 2;
    }
    else
    {
        mChannelCounts = 2;
    }
    
    mAudioHardware = hw;
    mDevices = devices; 

    return NO_ERROR;
}

status_t AudioStreamInASTER::setParameters(const String8& keyValuePairs)
{
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 key = String8(AudioParameter::keyRouting);
    status_t status = NO_ERROR;
    int device;

    LOGI("AudioStreamInASTER: setParameters: %s", keyValuePairs.string());

    if (param.getInt(key, device) == NO_ERROR)
    {
        LOGI("AudioStreamInASTER: set AudioStreamIn devices 0x%x", device);

        if (device & (device - 1)) 
        {
            status = BAD_VALUE;
        } 
        else 
        {
            mDevices = device;
            status = mAudioHardware->updateAudioDevices(this);
        }

        param.remove(key);
    }

    if (param.size()) 
    {
        status = BAD_VALUE;
    }

    return status;
}

String8 AudioStreamInASTER::getParameters(const String8& keys)
{
    AudioParameter param = AudioParameter(keys);
    String8 value;
    String8 key = String8(AudioParameter::keyRouting);

    if (param.get(key, value) == NO_ERROR) 
    {
        LOGI("AudioStreamInASTER: get AudioStreamIn devices 0x%x", mDevices);
        param.addInt(key, (int)mDevices);
    }

    LOGI("AudioStreamInASTER: getParameters: %s", param.toString().string());

    return param.toString();
}

ssize_t AudioStreamInASTER::read(void* buffer, ssize_t bytes)
{
    int   mode;

    if (mAudioHardware == NULL)
    {
        return bytes;
    }

    mode = mAudioHardware->getCurMode();

    //Recording voice call
    if (mode == AudioSystem::MODE_IN_CALL)
    {
        if (mAlsaHandle->status() == ALSAHandle::ALSA_NULL)
        {
            mAlsaHandle->open(ALSAHandle::ALSA_MONO_IN);
            mAlsaHandle->setHwParams(SND_PCM_FORMAT_S16_LE, mChannelCounts, sampleRate(), (snd_pcm_uframes_t) this->periodSize());
            mAlsaHandle->setSwParams(ALSAHandle::SW_RECORD);
        }

        if (mAlsaHandle->status() != ALSAHandle::ALSA_NULL)
        {
            mAlsaHandle->read(buffer, bytes);
        }
        return bytes;
    }
    else
    {
        if (mAlsaHandle->status() == ALSAHandle::ALSA_NULL)
        {
            mAlsaHandle->open(ALSAHandle::ALSA_MONO_IN);
            mAlsaHandle->setHwParams(SND_PCM_FORMAT_S16_LE, mChannelCounts, sampleRate(), (snd_pcm_uframes_t) this->periodSize());
            mAlsaHandle->setSwParams(ALSAHandle::SW_RECORD);
            mAudioHardware->setModeAndDevices(1, mode, devices());
        }

        if (mAlsaHandle->status() != ALSAHandle::ALSA_NULL)
        {
            mAlsaHandle->read(buffer, bytes);
        }
        return bytes;
    }

    return -1;
}

status_t AudioStreamInASTER::standby()
{
    LOGD("AudioStreamInASTER: standby");

    int mode = 0;

    if (mAudioHardware) 
    {
        mode = mAudioHardware->getCurMode();

        if (mode != AudioSystem::MODE_IN_CALL)
        {
            mAudioHardware->setModeAndDevices(0, mode, devices());
        }
    }

    if (mAlsaHandle)
    {
        mAlsaHandle->close();
    }

    return NO_ERROR;
}

void AudioStreamInASTER::resetFramesLost()
{
    mFramesLost = 0;
}

unsigned int AudioStreamInASTER::getInputFramesLost() const
{
    unsigned int count = mFramesLost;
    // Stupid interface wants us to have a side effect of clearing the count
    // but is defined as a const to prevent such a thing.
    ((AudioStreamInASTER *)this)->resetFramesLost();
    return count;
}

status_t AudioStreamInASTER::dump(int fd, const Vector<String16>& args) 
{ 
    const size_t SIZE = 256; 
    char buffer[SIZE]; 
    String8 result; 

    snprintf(buffer, SIZE, "AudioStreamInASTER::dump\n"); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tsample rate: %d\n", sampleRate()); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tbuffer size: %d\n", bufferSize()); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tchannel count: %d\n", mChannelCounts); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tformat: %d\n", format()); 
    result.append(buffer); 
    snprintf(buffer, SIZE, "\tmAudioHardware: %p\n", mAudioHardware); 
    result.append(buffer); 
    ::write(fd, result.string(), result.size()); 

    return NO_ERROR; 
} 

// ----------------------------------------------------------------------------
extern "C" AudioHardwareInterface* createAudioHardware(void) 
{
    return new AudioHardware();
}

}; // namespace android
