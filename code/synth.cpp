/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Johannes Spies $
   $Notice: (C) Copyright 2018. All Rights Reserved. $
   ======================================================================== */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <math.h>

#define Pi32 M_PI

typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;
typedef float f32;

int main(int ArgCount, char **Args)
{
    snd_pcm_t *PCM;
    int Error = snd_pcm_open(&PCM, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(Error == 0)
    {
        u32 Channels = 2;
        u32 SamplesPerSecond = 44100;
        snd_pcm_set_params(PCM, SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED, Channels, SamplesPerSecond, 1, 16667);
        snd_pcm_uframes_t RingBufferSize;
        snd_pcm_uframes_t PeriodSize;
        snd_pcm_get_params(PCM, &RingBufferSize, &PeriodSize);
        
        int PollDescCount = snd_pcm_poll_descriptors_count(PCM);
        pollfd *PollDescs = (pollfd *)malloc(PollDescCount * sizeof(pollfd));
        snd_pcm_poll_descriptors(PCM, PollDescs, PollDescCount);
        
        fprintf(stdout, "PCM config:\n");
        fprintf(stdout, "  Ring buffer size: %lu frames\n", RingBufferSize);
        u32 SampleBufferSize = PeriodSize * Channels * sizeof(f32);
        void *SampleBuffer = malloc(SampleBufferSize);
        fprintf(stdout, "  Period size:      %lu frames / %u bytes\n", PeriodSize, SampleBufferSize);
        fprintf(stdout, "  Poll desc. count: %d\n", PollDescCount);

        f32 f = 100.0f;
        u32 RunningSampleIndex = 0;
        fprintf(stdout, "Entering main loop.\n");
        for(;;)
        {
            int EventDescs = poll(PollDescs, PollDescCount, -1);
            if(EventDescs != -1)
            {
                u16 Events;
                snd_pcm_poll_descriptors_revents(PCM, PollDescs, PollDescCount, &Events);

                if(!(Events & POLLOUT))
                {
                    fprintf(stderr, "ERROR: Event is POLLOUT.\n");
                }                        

                f32 *SampleBufferPointer = (f32 *)SampleBuffer;
                for(u32 SampleIndex = 0;
                    SampleIndex < PeriodSize;
                    ++SampleIndex)
                {
                    f32 Volume = 0.4f;
                    f32 t = (f32)RunningSampleIndex / (f32)SamplesPerSecond;
                    f32 SampleValue = Volume * sinf(f * t * 2.0f * Pi32);
                    *SampleBufferPointer++ = SampleValue;
                    *SampleBufferPointer++ = SampleValue;

                    ++RunningSampleIndex;
                }

                int Error = snd_pcm_writei(PCM, SampleBuffer, PeriodSize);
                if(Error < 0)
                {
                    snd_pcm_recover(PCM, Error, 1);
                }
            }
            else
            {
                fprintf(stderr, "ERROR: Poll failed.\n");
            }
        }
        
        snd_pcm_close(PCM);
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to open pcm handle.\n");
    }
    
    return(0);
}
