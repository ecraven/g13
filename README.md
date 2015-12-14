* Userspace driver for the G13
** Installation
Make sure you have boost and libusb-1.0 installed.

Compile by running

> make

If you want to run the daemon as user, put the file 91-g13.rules into /etc/udev/rules.d/ (or whatever directory your distribution uses).

** Running
Connect your device, then run ./g13, it should automatically find your device.

If you see output like

Known keys on G13:
BD DOWN G1 G10 G11 G12 G13 G14 G15 G16 G17 G18 G19 G2 G20 G21 G22 G3 G4 G5 G6 G7 G8 G9 L1 L2 L3 L4 LEFT LIGHT LIGHT_STATE M1 M2 M3 MR TOP STICK_LEFT STICK_RIGHT STICK_UP STICK_DOWN 
Known keys to map to:
KEY_0 KEY_1 KEY_2 KEY_3 KEY_4 KEY_5 KEY_6 KEY_7 KEY_8 KEY_9 KEY_A KEY_APOSTROPHE KEY_B KEY_BACKSLASH KEY_BACKSPACE KEY_C KEY_CAPSLOCK KEY_COMMA KEY_D KEY_DOT KEY_DOWN KEY_E KEY_ENTER KEY_EQUAL KEY_ESC KEY_F KEY_F1 KEY_F10 KEY_F2 KEY_F3 KEY_F4 KEY_F5 KEY_F6 KEY_F7 KEY_F8 KEY_F9 KEY_G KEY_GRAVE KEY_H KEY_I KEY_J KEY_K KEY_KP0 KEY_KP1 KEY_KP2 KEY_KP3 KEY_KP4 KEY_KP5 KEY_KP6 KEY_KP7 KEY_KP8 KEY_KP9 KEY_KPASTERISK KEY_KPDOT KEY_KPMINUS KEY_KPPLUS KEY_L KEY_LEFT KEY_LEFTALT KEY_LEFTBRACE KEY_LEFTCTRL KEY_LEFTSHIFT KEY_M KEY_MINUS KEY_N KEY_NUMLOCK KEY_O KEY_P KEY_Q KEY_R KEY_RIGHT KEY_RIGHTBRACE KEY_RIGHTSHIFT KEY_S KEY_SCROLLLOCK KEY_SEMICOLON KEY_SLASH KEY_SPACE KEY_T KEY_TAB KEY_U KEY_UP KEY_V KEY_W KEY_X KEY_Y KEY_Z 
Found 1 G13s

that is good. This also shows you which name the keys on the G13 have, and what keys you can bind them to.

** Commands

The daemon creates a pipe at /tmp/g13-0, you can send commands via that pipe (e.g. by running "echo rgb 0 255 0 > /tmp/g13-0")

*** rgb <r> <g> <b>

Sets the background color

*** mod <n>

Sets the background light of the mod-keys. <n> is the sum of 1 (M1), 2 (M2), 4 (M3) and 8 (MR) (i.e. 13 
would set M1, M3 and MR, and unset M2).

*** bind <keyname> <action>

This binds a key or a stick zone. The possible values of <keyname> for keys are shown upon startup (e.g. G1).

    <action> can be a key, possible values shown upon startup  (e.g. KEY_LEFTSHIFT).
    
    <action> can be multiple keys,  like **bind G1 KEY_LEFTSHIFT+KEY_F1**

    <action> can be pipe output, by using ">" followed by text, as in **bind G3 >Hello** - causing "Hello\n" to be written to the output pipe ( /tmp/g13-0_out )

    <action> can be a command, by using "!" followed by text, as in **bind G4 !stick_mode KEYS** 

*** stickmode <mode>

The stick can be used as an absolute input device or can send key events. You can change modes to one of the following:

    **KEYS**        translates stick movements into key / action bindings
    
    **ABSOLUTE**
    
    **RELATIVE**
    
    **CALCENTER**   calibrate stick center position
    
    **CALBOUNDS**   calibrate stick boundaries
    
    **CALNORTH**    calibrate stick north
  
*** stickzone <operation> <zonename> <args>

where <operation> can be

     **add** add a new zone named **zonename**
     
     **del** remove zone named **zonename**
     
     **action** set action for zone 
     
     **bounds** set boundaries for zone, **args** are X1, Y1, X2, Y2, where X1/Y1 are top left, X2/Y2 are bottom right, values are floating point from 0.0 to 1.0 

Default created zones are LEFT, RIGHT, UP and DOWN.

*** LCD display

Use pbm2lpbm to convert a pbm image to the correct format, then just cat that into the pipe (cat starcraft2.lpbm > /tmp/g13-0).
The pbm file must be 160x43 pixels.

*** pos <row> <col>

Sets the current text position to <row> <col>

*** out <text>

Writes <text> to the LCD at the current text position

*** clear

Clears the LCD

*** textmode <mode>

Sets the text mode to <mode>, current options are 0 (normal) or 1 (inverted)

*** refresh

Resends the LCD buffer

*** profile <profile_name>
    
Selects <profile_name> to be the current profile, it if it doesn't exist creating it as a copy of the current profile.

All key binding changes (from the bind command) are made on the current profile.
 
 
*** font <font_name>   

Switch font, current options are 8x8 and 5x8    

* License
This code is placed in the public domain. Do with it whatever you want.