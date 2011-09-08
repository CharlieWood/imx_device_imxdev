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

#ifndef ANDROID_AUDIO_HARDWARE_H
#define ANDROID_AUDIO_HARDWARE_H

#include <stdint.h>
#include <sys/types.h>
#include <hardware_legacy/AudioHardwareBase.h>
#include <asoundlib.h>
#include "hwa.h"

namespace android {

// ----------------------------------------------------------------------------
class AudioHardware;

class ALSAHandle
{
public:
    typedef enum
    {
        SW_PLAY = 0,
        SW_RECORD
    } ALSA_SET_SW_FLAG;

    typedef struct 
    {
        snd_pcm_format_t format;
        unsigned int channels;
        unsigned int rate;
        snd_pcm_uframes_t periodSize;
        snd_pcm_uframes_t bufferSize;
        size_t bits_per_sample;
        size_t bits_per_frame;
        size_t bytes_per_frame;
    } hwParamType;

    typedef enum
    {
        ALSA_STEREO_OUT = 0,
        ALSA_MONO_OUT,
        ALSA_MONO_IN,
        ALSA_VOICE_CALL,

        ALSA_NULL = 0xFF
    } audioDeviceType;

    ALSAHandle();
    ~ALSAHandle();
    
    status_t open(audioDeviceType type = ALSA_STEREO_OUT);
    status_t setHwParams(snd_pcm_format_t format, unsigned int channels, unsigned int sampleRate, snd_pcm_uframes_t periodSize);
    status_t setSwParams(ALSA_SET_SW_FLAG flag);    
    void close();
    ssize_t write(const void *buffer, size_t bytes);
    ssize_t read(void *buffer, ssize_t bytes);
    audioDeviceType status();

private:
    ssize_t xrun(void);
    ssize_t suspend(void);
    
    snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
    snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
    
    audioDeviceType mDeviceType;
    
    snd_pcm_t* mPcmHandle;
    snd_pcm_stream_t mStreamType;
    hwParamType mHwparams;

#ifdef DUMP_PCM
    FILE *mPcmFile;
#endif
};

class AudioStreamOutASTER : public AudioStreamOut {
public:
                     AudioStreamOutASTER();
    virtual          ~AudioStreamOutASTER();

    status_t    set(AudioHardware* mHardware,
                    uint32_t devices,
                    int *pFormat,
                    uint32_t *pChannels,
                    uint32_t *pRate);

    virtual uint32_t    sampleRate() const;
    virtual size_t      bufferSize() const;
    virtual uint32_t    channels() const;
    virtual int         format() const { return AudioSystem::PCM_16_BIT; }
    size_t              periodSize() const { return 1536; }
    virtual uint32_t    latency() const { return 20; };
    virtual status_t    setVolume(float left, float right) { return INVALID_OPERATION; }
    virtual ssize_t     write(const void* buffer, size_t bytes);
    virtual status_t    standby();
    virtual status_t    dump(int fd, const Vector<String16>& args); 



    virtual status_t    setParameters(const String8& keyValuePairs);
    virtual String8     getParameters(const String8& keys);

    // return the number of audio frames written by the audio dsp to DAC since
    // the output has exited standby
    virtual status_t    getRenderPosition(uint32_t *dspFrames);

    uint32_t    devices() { return mDevices; }

private:
    AudioHardware   *mAudioHardware;
    ALSAHandle      *mAlsaHandle;

    uint32_t        mDevices;
    uint32_t 	    mSampleRate;
    size_t          mBufferSize;
    uint32_t        mChannels;
    int             mChannelCounts;
    uint32_t        mFrameCount;
};

class AudioStreamInASTER : public AudioStreamIn {
public:
                        AudioStreamInASTER();
    virtual             ~AudioStreamInASTER(); 

    status_t    set(
            AudioHardware *hw,
            uint32_t devices,
            int *pFormat,
            uint32_t *pChannels,
            uint32_t *pRate,
            AudioSystem::audio_in_acoustics acoustics);

    virtual uint32_t    sampleRate() const {return mInSampleRate; }
    virtual size_t      bufferSize() const { return 6144; }
    virtual uint32_t    channels() const { return AudioSystem::CHANNEL_IN_STEREO; }
    virtual int         format() const { return AudioSystem::PCM_16_BIT; }
    size_t              periodSize() const { return 1536; }
    virtual status_t    setGain(float gain) { return INVALID_OPERATION; }
    virtual ssize_t     read(void* buffer, ssize_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args); 
    virtual status_t    standby();

    virtual status_t    setParameters(const String8& keyValuePairs);
    virtual String8     getParameters(const String8& keys);

    // Return the amount of input frames lost in the audio driver since the last call of this function.
    // Audio driver is expected to reset the value to 0 and restart counting upon returning the current value by this function call.
    // Such loss typically occurs when the user space process is blocked longer than the capacity of audio driver buffers.
    // Unit: the number of input audio frames
    virtual unsigned int  getInputFramesLost() const;

            uint32_t    devices() { return mDevices; }

private:
    void            resetFramesLost();
    AudioHardware   *mAudioHardware;
    ALSAHandle      *mAlsaHandle;
    uint32_t        mDevices;
    uint32_t        mInSampleRate;
    int 	    mChannelCounts ;
    unsigned int    mFramesLost;
};


class AudioHardware : public  AudioHardwareBase
{
public:

                        AudioHardware();
    virtual             ~AudioHardware();
    virtual status_t    initCheck();
    virtual status_t    setVoiceVolume(float volume);
    virtual status_t    setMasterVolume(float volume);
    virtual size_t      getInputBufferSize(uint32_t sampleRate, int format, int channelCount);

    // mic mute
    virtual status_t    setMicMute(bool state);
    virtual status_t    getMicMute(bool* state);


    virtual status_t    setParameters(const String8& keyValuePairs);
    virtual String8     getParameters(const String8& keys);

    // create I/O streams
    virtual AudioStreamOut* openOutputStream(
                                uint32_t devices,
                                int *format=0,
                                uint32_t *channels=0,
                                uint32_t *sampleRate=0,
                                status_t *status=0); 

    virtual AudioStreamIn* openInputStream(
                                uint32_t devices,
                                int *format,
                                uint32_t *channels,
                                uint32_t *sampleRate,
                                status_t *status,
                                AudioSystem::audio_in_acoustics acoustics);

    virtual void        closeOutputStream(AudioStreamOut* out);
    virtual void        closeInputStream(AudioStreamIn* in);

    virtual status_t    setMode(int mode);

            status_t    setModeAndDevices(int on, int mode, uint32_t devices);

            int         getCurMode(void);
            uint32_t    getCurDevices(void);
            
            status_t    updateAudioDevices(AudioStreamInASTER* input);
            AudioStreamInASTER* getInputStream(void);
            
protected:
    virtual status_t    dump(int fd, const Vector<String16>& args); 

private:
    Mutex                 mLock;
    AudioStreamOutASTER   *mOutput;
    AudioStreamInASTER    *mInput;
    ALSAHandle            *mAlsaHandle;

    int             mCurMode;
    uint32_t        mCurDevices;
        
    unsigned int    mVoiceVolume; // CP volume, range from 0 -100
    bool            mMicMute;
    bool            mFirstEnableDevice;
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_AUDIO_HARDWARE_H
