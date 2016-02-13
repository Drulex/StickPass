/*
 * File: hid.h
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-13
 * License: GNU GPL v2 (see License.txt)
 *
 */

#ifndef HID_H
#define HID_H

#include <stdio.h>

typedef struct {
        uint8_t modifier;
        uint8_t reserved;
        uint8_t keycode;
} keyboard_report_t;

// function prototypes
void buildReport(unsigned char sendKey);
void clearKeyboardReport(void);

#endif

