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
    if(ArgCount == 3)
    {
        char *SoundDeviceName = Args[1];
        char *MIDIDeviceName = Args[2];
        
        snd_pcm_t *PCM;
        int Error = snd_pcm_open(&PCM, SoundDeviceName, SND_PCM_STREAM_PLAYBACK, 0);
        if(Error == 0)
        {
            u32 Channels = 2;
            u32 SamplesPerSecond = 48000;
            snd_pcm_set_params(PCM, SND_PCM_FORMAT_S16_LE,
                               SND_PCM_ACCESS_RW_INTERLEAVED, Channels, SamplesPerSecond, 1, 16667);
            snd_pcm_uframes_t RingBufferSize;
            snd_pcm_uframes_t PeriodSize;
            snd_pcm_get_params(PCM, &RingBufferSize, &PeriodSize);
        
            int PollDescCount = 1 + snd_pcm_poll_descriptors_count(PCM);
            pollfd *PollDescs = (pollfd *)malloc(PollDescCount * sizeof(pollfd));
            snd_pcm_poll_descriptors(PCM, PollDescs, PollDescCount - 1);
        
            fprintf(stdout, "PCM config:\n");
            fprintf(stdout, "  Ring buffer size: %lu frames\n", RingBufferSize);
            u32 SampleBufferSize = PeriodSize * Channels * sizeof(f32);
            void *SampleBuffer = malloc(SampleBufferSize);
            fprintf(stdout, "  Period size:      %lu frames / %u bytes\n", PeriodSize, SampleBufferSize);
            fprintf(stdout, "  Poll desc. count: %d\n", PollDescCount);

            int MIDIDeviceDesc = open(MIDIDeviceName, O_RDONLY);
            if(MIDIDeviceDesc != -1)
            {
                pollfd *PollMIDIDeviceDesc = PollDescs + PollDescCount - 1;
                PollMIDIDeviceDesc->fd = MIDIDeviceDesc;
                PollMIDIDeviceDesc->events = POLLIN;
                PollMIDIDeviceDesc->revents = 0;
            
                f32 f = 0.0f;
                u32 RunningSampleIndex = 0;
                fprintf(stdout, "Entering main loop.\n");
                for(;;)
                {
                    int PollResult = poll(PollDescs, PollDescCount, -1);
                    if(PollResult > 0)
                    {
                        if(PollMIDIDeviceDesc->revents & POLLIN)
                        {
                            char unsigned Buffer[4];
                            int ReadAmount = read(MIDIDeviceDesc, Buffer, 4);

                            switch(Buffer[0])
                            {
                                case 0x90:
                                {
                                    s32 Note = ((s32)Buffer[1] - 69);
                                    f = 440.0f * powf(2.0f, (f32)Note / 12.0f);
                                    fprintf(stdout, "Note on\n");
                                } break;

                                case 0x80:
                                {
                                    f = 0.0f;
                                    fprintf(stdout, "Note off\n");
                                } break;

                                default:
                                {
                                    fprintf(stderr, "Unhandled command: %02x.\n", Buffer[0]);
                                } break;
                            }
                        }

                        u16 EventToHandle;
                        snd_pcm_poll_descriptors_revents(PCM,
                                                         PollDescs,
                                                         PollDescCount - 1,
                                                         &EventToHandle);
                        if(EventToHandle & POLLOUT)
                        {                        
                            s16 *SampleBufferPointer = (s16 *)SampleBuffer;
                            for(u32 SampleIndex = 0;
                                SampleIndex < PeriodSize;
                                ++SampleIndex)
                            {
                                f32 Volume = 0.4f;
                                f32 t = (f32)RunningSampleIndex / (f32)SamplesPerSecond;
                                s16 SampleValue = (s16)(32768.0f * (Volume * sinf(f * t * 2.0f * Pi32)));
                                *SampleBufferPointer++ = SampleValue;
                                *SampleBufferPointer++ = SampleValue;

                                ++RunningSampleIndex;
                                if(RunningSampleIndex > SamplesPerSecond)
                                {
                                    RunningSampleIndex = 0;
                                }
                            }

                            int Error = snd_pcm_writei(PCM, SampleBuffer, PeriodSize);
                            if(Error < 0)
                            {
                                snd_pcm_recover(PCM, Error, 1);
                            }
                        }
                    }
                }
            }
            else
            {
                fprintf(stderr, "ERROR: Failed to open MIDI device \"%s\".\n", MIDIDeviceName);
            }
            
            snd_pcm_close(PCM);
        }
        else
        {
            fprintf(stderr, "ERROR: Failed to open pcm handle.\n");
        }
    }
    else
    {
        fprintf(stdout, "Usage: %s <sound sink name> <midi device name>\n",
                Args[0]);
    }
    
    return(0);
}
