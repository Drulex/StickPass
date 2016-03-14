# STICKPASS

## What is the StickPass project?
StickPass is a hardware password manager based on the ATTINY85 microcontroller. It allows the user to securely store his identities (username/password combinations) on a physical device and use them with the press of a button. At the moment it uses the V-USB library to implement USB functionality in software.

## Scope
The scope of the project is to develop a low cost integrated hardware password management solution. The project is realized as a final year electrical engineering project and the timeline is 12 weeks at about 9 hours of work per week.

## Why ATTINY85?
There are two obvious ways of going when thinking about the architecture of this project:

1. Use an IC with integrated USB.
2. Use a general purpose IC and a separate chip acting as USB bridge.

Yet I chose to explore a third more obscure option: Use a software implementation of the USB protocol.


The reasons for this decision are the following:
* V-USB if fully compliant with USB1.1.
* ATTINY85 is an extremely small package (8 pin DIP package). There are no ICs with integrated USB that are this small.
* Very cheap (~1.67$) compared to other ICs with USB integrated or compared to using two ICs.
* Hardware is greatly simplified as the IC is powered directly from USB VCC and does not require a single external component (not even oscillator).
* This project does not require a lot of CPU and memory resources nor does it need full speed USB protocol.
* AVR chips are very well documented and used in various projects in the online community.
* Need to go from nothing to "production ready" in less than 150 hours.


## Compiling the firmware
At the moment this was only tested on Linux. Cross platform support is coming.

#### Linux
The following packages are required in order to build the firmware:
* avr-libc
* arvdude
* binutils-avr
* gcc-avr

If you are not using the USBTINY programmer to flash your ATTINY85 modify line 6 of the Makefile to call your programmer.

To compile the project: ``` make hex ```

To flash the chip: ``` make flash ```

To flash the fuses: ``` make fuse ```

#### OSX
Coming soon.

#### Windows
Coming soon... or not.


## Compiling the user application
In order to use the user application (StickApp) you will need a recent version of:
* gcc
* libusb

To compile the app: ``` make app ```


## Using the StickApp

#### Device initialization
```./stickapp --init <unlock key> ```

This will initialize the device with the given unlock key. This key must be used to unlock the device any time it is plugged in the future!
When used on an already initialized device it will wipe the memory contents and reset the key.

#### Send credentials to device
```./stickapp --send <idName> <idUsername> <idPassword> ```
* idName: nickname associated with credential
* idUsername: username associated with credential
* idPassword: password associated with credential

#### Clearing the EEPROM
``` ./stickapp --clear ```
Will clear the memory contents and preserve the unlock key.

#### Using credentials
To use the device:

1. Plug in computer. The LED should light up and stay solid. This means the device is locked.
2. Unlock the device using the unlock key.
3. A pushbutton shortpress (lesser than 1 second) will display and iterate through available idNames.
4. A pushbutton long press (greater than 1 second) will inject the idUsername, the TAB character and the idPassword.

## Limitations
Some decisions were made to implement some features (most of them related to memory management) with limitations in order to satisfy the requirements, but at the same time decrease complexity and ultimately save some time. I am obviously aware that these implementations are suboptimal and I plan on fixing them as soon as the semester is done and time allows.

##### Current limitations on version 1.0:
1. Maximum of 8 credentials capacity as per scheme below:
   * idName: 10 bytes
   * idUsername: 32 bytes
   * idPassword: 21 bytes
2. Unlock key size is 7 bytes.
3. No mechanisms implemented to prevent EEPROM corruption. This means that if you remove power during an EEPROM write cycle your data WILL be corrupted.

## Next version
I already have some ideas for the next iteration the main ones being:

1. External memory chip with hardware encryption.
2. NFC support for 2 factor authentication.
3. Rotating knob for navigation.
4. Tiny OLED matrix display.

If you have suggestions contact me at alexandru@jora.ca I would love to get some feedback.
