/*
 * File: hid.h
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-13
 * License: GNU GPL v3 (see LICENSE)
 *
 */

#ifndef HID_H
#define HID_H

#include <stdio.h>
#define KEY_BS  0x08
#define KEY_TAB 0x09

typedef struct {
        uint8_t modifier;
        uint8_t reserved;
        uint8_t keycode;
} keyboard_report_t;

// function prototypes
void buildReport(unsigned char sendKey);
void clearKeyboardReport(void);

#endif

