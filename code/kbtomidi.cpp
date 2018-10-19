/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Johannes Spies $
   $Notice: (C) Copyright 2018. All Rights Reserved. $
   ======================================================================== */
 
#define global static 
#define internal static

#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <stdio.h>

global bool GlobalKeyStates[18] = {};
global int GlobalMIDIFileDesc;

internal void
UpdateKeyState(int KeyIndex, bool State)
{
    if(State != GlobalKeyStates[KeyIndex])
    {
        GlobalKeyStates[KeyIndex] = State;

        char unsigned MessagePacket[3] = {};
        if(State)
        {
            MessagePacket[0] = 0x90;
        }
        else
        {
            MessagePacket[0] = 0x80;
        }
        MessagePacket[1] = KeyIndex + 58;
        
        write(GlobalMIDIFileDesc, MessagePacket, sizeof(MessagePacket));

        fprintf(stdout, "Note #%d is now %s.\n",
                KeyIndex, State ? "on" : "off");
    }
}

int main(int ArgCount, char **Args)
{
    if(ArgCount == 2)
    {
        char *MIDIPath = Args[1];
        GlobalMIDIFileDesc = open(MIDIPath, O_WRONLY);
        if(GlobalMIDIFileDesc)
        {
            Display *DisplayHandle = XOpenDisplay(0);
            if(DisplayHandle)
            {
                // NOTE(johannes): Disable key repeats...
                XAutoRepeatOff(DisplayHandle);
                
                int Screen = DefaultScreen(DisplayHandle);
                Window WindowHandle =
                    XCreateSimpleWindow(DisplayHandle, RootWindow(DisplayHandle, Screen),
                                        10, 10, 100, 100, 1,
                                        BlackPixel(DisplayHandle, Screen),
                                        WhitePixel(DisplayHandle, Screen));
                if(WindowHandle)
                {
                    XSelectInput(DisplayHandle, WindowHandle,
                                 ExposureMask|KeyPressMask|KeyReleaseMask);
                    XMapWindow(DisplayHandle, WindowHandle);

                    bool Running = true;
                    while(Running)
                    {
                        XEvent EventHandle;
                        XNextEvent(DisplayHandle, &EventHandle);

                        switch(EventHandle.type)
                        {
                            case Expose:
                            {
                                XFillRectangle(DisplayHandle, WindowHandle,
                                               DefaultGC(DisplayHandle, Screen),
                                               20, 20, 10, 10);
                            } break;

                            case KeyPress:
                            case KeyRelease:
                            {
                                bool Operation = (EventHandle.type == KeyPress);
                        
                                switch(EventHandle.xkey.keycode)
                                {
                                    case 0x09:
                                    {
                                        Running = false;
                                    } break;

                                    case 0x26:
                                    {UpdateKeyState(0, Operation);} break;
                            
                                    case 0x19:
                                    {UpdateKeyState(1, Operation);} break;
                            
                                    case 0x27:
                                    {UpdateKeyState(2, Operation);} break;

                                    case 0x1a:
                                    {UpdateKeyState(3, Operation);} break;

                                    case 0x28:
                                    {UpdateKeyState(4, Operation);} break;
                            
                                    case 0x29:
                                    {UpdateKeyState(5, Operation);} break;
                            
                                    case 0x1c:
                                    {UpdateKeyState(6, Operation);} break;
                            
                                    case 0x2a:
                                    {UpdateKeyState(7, Operation);} break;
                            
                                    case 0x1d:
                                    {UpdateKeyState(8, Operation);} break;
                            
                                    case 0x2b:
                                    {UpdateKeyState(9, Operation);} break;
                            
                                    case 0x1e:
                                    {UpdateKeyState(10, Operation);} break;
                            
                                    case 0x2c:
                                    {UpdateKeyState(11, Operation);} break;
                            
                                    case 0x2d:
                                    {UpdateKeyState(12, Operation);} break;
                            
                                    case 0x20:
                                    {UpdateKeyState(13, Operation);} break;
                            
                                    case 0x2e:
                                    {UpdateKeyState(14, Operation);} break;
                            
                                    case 0x21:
                                    {UpdateKeyState(15, Operation);} break;
                            
                                    case 0x2f:
                                    {UpdateKeyState(16, Operation);} break;
                            
                                    case 0x30:
                                    {UpdateKeyState(17, Operation);} break;
                            
                                    default:
                                    {
                                        fprintf(stdout, "Key %d = 0x%x pressed.\n",
                                                EventHandle.xkey.keycode,
                                                EventHandle.xkey.keycode);
                                    } break;
                                }
                            } break;
                    
                            default:
                            {
                                fprintf(stderr, "Unhandled X11 event.\n");
                            } break;
                        }
                    }

                    XAutoRepeatOn(DisplayHandle);
                    XDestroyWindow(DisplayHandle, WindowHandle);
                    XCloseDisplay(DisplayHandle);
                }
                else
                {
                    fprintf(stderr, "ERROR: Failed to create X11 window.\n");
                }
            }
            else
            {
                fprintf(stderr, "ERROR: Failed to connect to X11 display server.\n");
            }
        }
        else
        {
            fprintf(stderr, "ERROR: Unable to open MIDI stream for writing.\n");
        }
    }
    else
    {
        fprintf(stdout, "Usage: %d <midi stream name>\n", Args[0]);
    }
    
    return(0);
}
