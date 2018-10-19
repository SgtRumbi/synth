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
#include <fcntl.h>
#include <sys/poll.h>
//#include <alsa/asoundlib.h>
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
    if(ArgCount == 2)
    {
#if 1
        int MIDIDeviceDesc = open(Args[1], O_RDONLY);
        if(MIDIDeviceDesc != -1)
        {
            pollfd PollMIDIDeviceDesc = {};
            PollMIDIDeviceDesc.fd = MIDIDeviceDesc;
            PollMIDIDeviceDesc.events = POLLIN;
            PollMIDIDeviceDesc.revents = 0;
            
            for(;;)
            {
                int PollResult = poll(&PollMIDIDeviceDesc, 1, -1);
                
                if((PollResult > 0) &&
                   (PollMIDIDeviceDesc.revents & POLLIN))
                {
                    char unsigned Buffer[4];
                    int ReadAmount = read(MIDIDeviceDesc, Buffer, 4);

                    printf("Message:\n");
                    printf("  Raw: %02x %02x %02x %02x\n",
                           Buffer[0], Buffer[1], Buffer[2], Buffer[3]);
                    printf("  Key: %d\n",
                           Buffer[1]);
                }
                else
                {
                    //printf("Update\n");
                }
            }
            
            close(MIDIDeviceDesc);
        }
        else
        {
            fprintf(stderr, "ERROR: Failed to open MIDI device.\n");
        }    
#else
        snd_rawmidi_t *MIDIInput;
        int Error = snd_rawmidi_open(&MIDIInput, 0, Args[1], 0);
        if(Error == 0)
        {
            snd_rawmidi_nonblock(MIDIInput, 1);
            snd_rawmidi_close(MIDIInput); 
        }
        else
        {
            fprintf(stderr, "ERROR: Failed to open MIDI device.\n");
        }
#endif
    }
    else
    {
        fprintf(stdout, "Usage: %s <midi device name>\n", Args[0]);
    }
    
    return(0);
}
