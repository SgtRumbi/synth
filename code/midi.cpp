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
    snd_rawmidi_t *MIDIInput;
    int Error = snd_rawmidi_open(&MIDIInput, 0, "default", 0);
    if(Error == 0)
    {
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to open MIDI device.\n");
    }
    
    return(0);
}
