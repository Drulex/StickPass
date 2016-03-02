/*
 * File: main.h
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-28
 * License: GNU GPL v2 (see License.txt)
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

#include "usbdrv.h"
#include "credentials.h"
#include "led.h"
#include "hid.h"
#include "timer1.h"

// states for id cycling and injection
#define STATE_WAIT 0
#define STATE_INIT 1
#define STATE_SEND_ID_NAME 2
#define STATE_RELEASE_ID_NAME 3
#define STATE_LONG_KEY 4
#define STATE_SEND_ID_USERNAME 5
#define STATE_RELEASE_ID_USERNAME 6
#define STATE_SEND_TAB 7
#define STATE_RELEASE_TAB 8
#define STATE_SEND_ID_PASSWORD 9
#define STATE_RELEASE_ID_PASSWORD 10

// states for usb_msg parsing
#define USB_LED_OFF 0
#define USB_LED_ON 1
#define USB_CLEAR_EEPROM 2
#define USB_ID_UPLOAD 3
#define USB_UNLOCK_DEVICE 15
#define USB_INIT_DEVICE 16

// states for usbFunctionWrite
#define STATE_ID_UPLOAD_INIT 4
#define STATE_ID_NAME_SEND 5
#define STATE_ID_NAME_DONE 6
#define STATE_ID_USERNAME_SEND 7
#define STATE_ID_USERNAME_DONE 8
#define STATE_ID_PASS_SEND 9
#define STATE_ID_PASS_DONE 10
#define STATE_ID_UPLOAD_DONE 11
#define STATE_UNLOCK_DEVICE 12
#define STATE_INIT_DEVICE 13

// ASCII key codes for BS and TAB keys
#define KEY_BS  0x08
#define KEY_TAB 0x09

// init
static unsigned char pbCounter = 0;
static unsigned char state = STATE_WAIT;
static unsigned char flagDone = 0;
static unsigned char flagCredReady = 0;
static unsigned char flagKeyCleared = 1;
static unsigned char flagUnlocked = 0;
static unsigned char idMsgPtr = 0;
static unsigned char idState;
static unsigned char clearKeyCnt = 0;
static unsigned char idCnt = 0;
static unsigned char credPtr = 0;
static char masterKey[7];

cred_t credReceived;
keyboard_report_t keyboard_report;

volatile static unsigned char LED_state = 0xff;
static unsigned char idleRate;

// for PB long press detection
static unsigned char pbHold = 0;

// hid descriptor stored in flash
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)(224)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)(231)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs) ; Modifier byte
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) ; Reserved byte
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs) ; LED report
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs) ; LED report padding
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))(0)
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)(101)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};
