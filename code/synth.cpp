/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Johannes Spies $
   $Notice: (C) Copyright 2018. All Rights Reserved. $
   ======================================================================== */

#define Maximum(A, B) (((A) > (B)) ? (A) : (B))
#define Minimum(A, B) (((A) < (B)) ? (A) : (B))

#define internal static

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
typedef u32 b32;

struct midi_event
{
    union
    {
        struct
        {
            u8 Status_Channel;
            u8 Data0;
            u8 Data1;
        };
        u8 E[3];
    };

    midi_event *Next;
};

struct platform_input
{
    midi_event *FirstEvent;
    midi_event *LastEvent;
    midi_event *FirstFreeEvent;
};

struct voice
{
    f32 f;
    b32 Active;
    u32 SampleIndex;
};

internal f32
Envelope(u32 SampleIndex)
{
    f32 Result;

    u32 AttackTime = 24000 / 2;
    if(SampleIndex < AttackTime)
    {
        Result = 1.0f / (f32)AttackTime * (f32)SampleIndex;
    }
    else
    {
        Result = Maximum(0.0f, 1.0f - 1.0f / (f32)AttackTime * (f32)(SampleIndex - AttackTime));
    }

    return(Result);
}

internal void
Update(void *SampleBuffer, u32 SamplesToOutput, u32 SamplesPerSecond, platform_input *Input)
{
    static voice Voices[108 - 28] = {};
    
    for(midi_event *Event = Input->FirstEvent;
        Event;
        Event = Event->Next)
    {
        switch(Event->E[0])
        {
            case 0x90:
            {
                voice *Voice = Voices + Event->E[1] - 28;
                
                s32 Note = ((s32)Event->E[1] - 69);
                Voice->f =
                    440.0f * powf(2.0f, (f32)Note / 12.0f);
                Voice->SampleIndex = 0;
                Voice->Active = true;
                fprintf(stdout, "Note on\n");
            } break;

            case 0x80:
            {
                voice *Voice = Voices + Event->E[1] - 28;

                Voice->Active = false;
                fprintf(stdout, "Note off\n");
            } break;

            default:
            {
                fprintf(stderr, "Unhandled command: %02x.\n", Event->E[0]);
            } break;
        }
    }

    if(Input->FirstEvent)
    {
        Input->LastEvent->Next = Input->FirstFreeEvent;
        Input->FirstFreeEvent = Input->FirstEvent;
        Input->FirstEvent = Input->LastEvent = 0;
    }
    
    s16 *SampleBufferPointer = (s16 *)SampleBuffer;

    for(u32 SampleIndex = 0;
        SampleIndex < SamplesToOutput;
        ++SampleIndex)
    {
        f32 Volume = 0.01f;
        f32 SampleValueSum = 0.0f;

        for(u32 VoiceIndex = 0;
            VoiceIndex < (108 - 28);
            ++VoiceIndex)
        {
            voice *Voice = Voices + VoiceIndex;

            f32 t = (f32)Voice->SampleIndex / (f32)SamplesPerSecond;
            SampleValueSum += (Volume * Envelope(Voice->SampleIndex) * sinf(Voice->f * t * 2.0f * Pi32));
            
            ++Voice->SampleIndex;
        }
        
        s16 SampleValue16 = (s16)(32768.0f * SampleValueSum);
        *SampleBufferPointer++ = SampleValue16;
        *SampleBufferPointer++ = SampleValue16;
    }
}

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

                platform_input Input = {};
                
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
                            u8 Buffer[3];
                            int ReadAmount = read(MIDIDeviceDesc, Buffer, 4);

                            midi_event *Event;
                            if(Input.FirstFreeEvent)
                            {
                                Event = Input.FirstFreeEvent;
                                Input.FirstFreeEvent = Event->Next;
                            }
                            else
                            {
                                Event = (midi_event *)
                                    malloc(sizeof(midi_event));
                            }                       
                                 
                            memcpy(Event->E, Buffer, 3);

                            Event->Next = 0;
                            if(Input.FirstEvent)
                            {
                                Input.LastEvent->Next = Event;
                            }
                            else
                            {
                                Input.FirstEvent = Input.LastEvent = Event;
                            }
                        }

                        u16 EventToHandle;
                        snd_pcm_poll_descriptors_revents(PCM,
                                                         PollDescs,
                                                         PollDescCount - 1,
                                                         &EventToHandle);
                        if(EventToHandle & POLLOUT)
                        {                        
                            Update(SampleBuffer, PeriodSize, SamplesPerSecond, &Input);
                            
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
