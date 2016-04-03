/*
 * File: hid.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-13
 * License: GNU GPL v3 (see LICENSE)
 *
 */

#include "hid.h"

extern keyboard_report_t keyboard_report;

void buildReport(unsigned char sendKey) {
    // by default no modifier
    keyboard_report.modifier = 0;

    // handle empty report
    if(sendKey == 0) {
        keyboard_report.keycode = 0;
        return;
    }

    // handle backspace
    if(sendKey == KEY_BS) {
        keyboard_report.modifier = 2;
        keyboard_report.keycode = 0x2A;
        return;
    }

    // handle TAB
    if(sendKey == KEY_TAB) {
        keyboard_report.keycode = 0x2B;
        return;
    }

    // handle lowercase chars
    if(sendKey >= 'a' && sendKey <= 'z') {
        keyboard_report.keycode = 4 + (sendKey - 'a');
        return;
    }

    // handle uppercase chars
    if(sendKey >= 'A' && sendKey <= 'Z') {
        keyboard_report.modifier = 2;
        keyboard_report.keycode = 4 + (sendKey - 'A');
        return;
    }

    // handle digits 1 to 9
    if(sendKey >= '1' && sendKey <= '9') {
        keyboard_report.keycode = 30 + (sendKey - '1');
        return;
    }

    // handle rest of chars
    else {
        keyboard_report.modifier = 2;
        switch(sendKey) {

            case '0':
                keyboard_report.modifier = 0;
                keyboard_report.keycode = 0x27;
                break;

            case '@':
                keyboard_report.keycode = 0x1F;
                break;

            case '#':
                keyboard_report.keycode = 0x20;
                break;

            case '$':
                keyboard_report.keycode = 0x21;
                break;

            case '%':
                keyboard_report.keycode = 0x22;
                break;

            case '^':
                keyboard_report.keycode = 0x23;
                break;

            case '&':
                keyboard_report.keycode = 0x24;
                break;

            case '*':
                keyboard_report.keycode = 0x25;
                break;

            case '(':
                keyboard_report.keycode = 0x26;
                break;

            case ')':
                keyboard_report.keycode = 0x27;
                break;

            case '_':
                keyboard_report.keycode = 0x2D;
                break;

            case '.':
                keyboard_report.modifier = 0;
                keyboard_report.keycode = 0x37;
                break;

            case ' ':
                keyboard_report.modifier = 0;
                keyboard_report.keycode = 0x2C;
                break;

            // send NULL for unsupported chars
            default:
                keyboard_report.keycode = 0;
                break;
        }
    }
}

void clearKeyboardReport(void) {
    unsigned char i;
    for(i = 0; i < sizeof(keyboard_report); i++) {
        ((unsigned char *)&keyboard_report)[i] = 0;
    }
}
