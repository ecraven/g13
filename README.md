# Userspace driver for the G13

## Installation

Make sure you have boost and libusb-1.0 installed.

Compile by running

    make

If you want to run the daemon as user, put the file 91-g13.rules into /etc/udev/rules.d/ (or whatever directory your distribution uses).

## Running

Connect your device, then run ./g13, it should automatically find your device.

If you see output like

    Known keys on G13:
    BD DOWN G1 G10 G11 G12 G13 G14 G15 G16 G17 G18 G19 G2 G20 G21 G22 G3 G4 G5 G6 G7
     G8 G9 L1 L2 L3 L4 LEFT LIGHT LIGHT2 LIGHT_STATE M1 M2 M3 MISC_TOGGLE MR TOP UND
    EF1 UNDEF3 
    Known keys to map to:
    0 1 2 3 4 5 6 7 8 9 A APOSTROPHE B BACKSLASH BACKSPACE C CAPSLOCK COMMA D DELETE
     DOT DOWN E END ENTER EQUAL ESC F F1 F10 F11 F12 F2 F3 F4 F5 F6 F7 F8 F9 G GRAVE
      H HOME I INSERT J K KP0 KP1 KP2 KP3 KP4 KP5 KP6 KP7 KP8 KP9 KPASTERISK KPDOT K
      PMINUS KPPLUS L LEFT LEFTALT LEFTBRACE LEFTCTRL LEFTSHIFT M MINUS N NUMLOCK O 
      P PAGEDOWN PAGEUP Q R RIGHT RIGHTALT RIGHTBRACE RIGHTCTRL RIGHTSHIFT S SCROLLL
    OCK SEMICOLON SLASH SPACE T TAB U UP V W X Y Z 
    Found 1 G13s
    
    Active Stick zones 
               STICK_UP   { 0 x 0.1 / 1 x 0.3 }   SEND KEYS: UP
             STICK_DOWN   { 0 x 0.7 / 1 x 0.9 }   SEND KEYS: DOWN
             STICK_LEFT   { 0 x 0 / 0.2 x 1 }   SEND KEYS: LEFT
            STICK_RIGHT   { 0.8 x 0 / 1 x 1 }   SEND KEYS: RIGHT
           STICK_PAGEUP   { 0 x 0 / 1 x 0.1 }   SEND KEYS: PAGEUP
         STICK_PAGEDOWN   { 0 x 0.9 / 1 x 1 }   SEND KEYS: PAGEDOWN



that is good. This also shows you which name the keys on the G13 have, and what keys you can bind them to.

## Configuring / Remote Control

The daemon creates a pipe at /tmp/g13-0, you can send commands via that pipe (e.g. by running *"echo rgb 0 255 0 > /tmp/g13-0"*)

## Commands

### rgb <r> <g> <b>

Sets the background color

### mod <n>

Sets the background light of the mod-keys. <n> is the sum of 1 (M1), 2 (M2), 4 (M3) and 8 (MR) (i.e. 13 
would set M1, M3 and MR, and unset M2).

### bind <keyname> <action>

This binds a key or a stick zone. The possible values of <keyname> for keys are shown upon startup (e.g. G1).

    <action> can be a key, possible values shown upon startup  (e.g. KEY_LEFTSHIFT).
    
    <action> can be multiple keys,  like **bind G1 KEY_LEFTSHIFT+KEY_F1**

    <action> can be pipe output, by using ">" followed by text, as in **bind G3 >Hello** - causing "Hello\n" to be written to the output pipe ( /tmp/g13-0_out )

    <action> can be a command, by using "!" followed by text, as in **bind G4 !stick_mode KEYS** 

### stickmode <mode>

The stick can be used as an absolute input device or can send key events. You can change modes to one of the following:

Mode       | Description
-----------|---------------------------
KEYS       | translates stick movements into key / action bindings
ABSOLUTE   |
RELATIVE   |
CALCENTER  | calibrate stick center position
CALBOUNDS  | calibrate stick boundaries
CALNORTH   | calibrate stick north
  
### stickzone <operation> <zonename> <args>

where <operation> can be

operation | what it does
----------|----------------
add       | add a new zone named **zonename**
del       | remove zone named **zonename**
action    | set action for zone 
bounds    | set boundaries for zone, **args** are X1, Y1, X2, Y2, where X1/Y1 are top left, X2/Y2 are bottom right, values are floating point from 0.0 to 1.0 

Default created zones are LEFT, RIGHT, UP and DOWN.


### pos <row> <col>

Sets the current text position to <row> <col>

### out <text>

Writes <text> to the LCD at the current text position

### clear

Clears the LCD

### textmode <mode>

Sets the text mode to <mode>, current options are 0 (normal) or 1 (inverted)

### refresh

Resends the LCD buffer

### profile <profile_name>
    
Selects <profile_name> to be the current profile, it if it doesn't exist creating it as a copy of the current profile.

All key binding changes (from the bind command) are made on the current profile.
 
 
### font <font_name>   

Switch font, current options are 8x8 and 5x8    

### LCD display

Use pbm2lpbm to convert a pbm image to the correct format, then just cat that into the pipe (cat starcraft2.lpbm > /tmp/g13-0).
The pbm file must be 160x43 pixels.

## License

All files without a copyright notice are public domainplaced in the public domain. Do with it whatever you want.

Some source code files include MIT style license - see files for specifics.