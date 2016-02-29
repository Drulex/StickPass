/*
 * File: main.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-27
 * License: GNU GPL v2 (see License.txt)
 *
 */

#include "main.h"

/*
 * This is called when the host send a usb_msg on control enpoint 0
 * It parses requests made by the host which can be HID related (required by spec)
 * or requests defined by us.
 */
usbMsgLen_t usbFunctionSetup(unsigned char data[8]) {
    usbRequest_t *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        switch(rq->bRequest) {

            //send NO_KEYS_PRESSED
            case USBRQ_HID_GET_REPORT:
                usbMsgPtr = (void *)&keyboard_report;
                keyboard_report.modifier = 0;
                keyboard_report.keycode = 0;

                return sizeof(keyboard_report);

            case USBRQ_HID_SET_REPORT: // if wLength == 1, should be LED state
                return (rq->wLength.word == 1) ? USB_NO_MSG : 0;

            // send idleRate to PC as per spec
            case USBRQ_HID_GET_IDLE: // send idle rate to PC as required by spec
                usbMsgPtr = &idleRate;
                return 1;

            // set idleRate as per spec
            case USBRQ_HID_SET_IDLE: // save idle rate as required by spec
                idleRate = rq->wValue.bytes[1];
                return 0;
        }
    }
    // other requests
    else {
        switch(rq->bRequest) {
            case USB_UNLOCK_DEVICE:
                return USB_NO_MSG;

            case USB_LED_ON:
                if(flagUnlocked)
                    LED_HIGH();
                return 0;

            case USB_LED_OFF:
                if(flagUnlocked)
                    LED_LOW();
                return 0;

            case USB_ID_UPLOAD:
                if(flagUnlocked)
                    return USB_NO_MSG;
                else
                    return 0;

            case USB_CLEAR_EEPROM:
                if(flagUnlocked) {
                    clearEEPROM();
                    idCnt = 0;
                }
                return 0;
        }
    }
    return 0;
}

/*
 * This function is called when usbFunctionSetup return USB_NO_MSG
 * We can only receive chunks of 8 bytes so the state value
 * is stored in the first byte of the data buffer
 *
 */
usbMsgLen_t usbFunctionWrite(uint8_t * data, unsigned char len) {
    unsigned char i;
    idState = data[0];
    switch(idState) {
        case STATE_UNLOCK_DEVICE:
            //getMasterKey(&masterKey);
            if(memcmp(masterKey, &data[1], 7) == 0) {
                flagUnlocked = 1;
                LED_LOW();
            }
            return 1;

        case STATE_ID_UPLOAD_INIT:
            // clear credentials structure and reset msg pointer
            clearCred(&credReceived);
            idMsgPtr = 0;
            return 1;

        case STATE_ID_NAME_SEND:
            for(i = 1; i < len; i++) {
                credReceived.idName[idMsgPtr] = data[i];
                idMsgPtr++;
            }
            return 1;

        case STATE_ID_NAME_DONE:
            idMsgPtr = 0;
            return 1;

        case STATE_ID_USERNAME_SEND:
            for(i = 1; i < len; i++) {
                credReceived.idUsername[idMsgPtr] = data[i];
                idMsgPtr++;
            }
            return 1;

        case STATE_ID_USERNAME_DONE:
            idMsgPtr = 0;
            return 1;

        case STATE_ID_PASS_SEND:
            for(i = 1; i < len; i++) {
                credReceived.idPassword[idMsgPtr] = data[i];
                idMsgPtr++;
            }
            return 1;

        case STATE_ID_PASS_DONE:
            flagCredReady = 1;
            update_credential(credReceived);
            return 1;
    }

    return 1;
}

int main() {
    // variables declaration
    unsigned char i;
    cred_t cred;

    // Modules initialization
    LED_Init();
    timer1_Init();
    PB_INIT();
    LED_HIGH();

    // variables init
    clearKeyboardReport();
    clearCred(&cred);

    // global interrupts off
    cli();

    // get number of credentials in EEPROM
    getCredCount();
    idCnt = credCount;

    // initialize usb library
    usbInit();

    // enforce re-enumeration
    usbDeviceDisconnect();
    for(i = 0; i<250; i++) {
        wdt_reset();
        _delay_ms(2);
    }

    usbDeviceConnect();

    LED_LOW();
    _delay_ms(500);
    LED_HIGH();

    // Enable 1s watchdog
    wdt_enable(WDTO_1S);

    // Enable global interrupts after re-enumeration
    sei();

    while(1) {
        wdt_reset();
        usbPoll();

        // only if device is unlocked
        if(flagUnlocked == 1) {
            // PB press detection
            if(!(PINB & (1<<PB3))) {
                // debouncing
                if(state == STATE_WAIT && pbCounter == 255) {
                    // short PB press
                    state = STATE_INIT;
                    flagDone = 0;// reset counter

                    // iterate to next idCnt
                    if(idCnt < credCount) {
                        idCnt++;
                    }
                    else
                        idCnt = 1;

                    counter100ms = 0;

                    while(!(PINB & (1<<PB3)) && !pbHold) {
                        // waiting for PB long press event
                        wdt_reset();
                        // if PB is held for 1.0s
                        if(counter100ms >= 10) {
                            counter100ms = 0;
                            state = STATE_LONG_KEY;
                            // send idUsername
                            pbHold = 1;
                            flagDone = 0;
                        }
                    }
                }
                pbHold = 0;
                pbCounter = 0;
            }
            // debouncing
            if(pbCounter < 255)
                pbCounter++;

            if(usbInterruptIsReady() && state != STATE_WAIT && !flagDone) {
                unsigned char sendKey;
                switch(state) {
                    case STATE_INIT:
                        clearCred(&cred);
                        getCredentialData(idCnt, &cred);
                        credPtr = 0;
                        state = STATE_SEND_ID_NAME;
                        flagKeyCleared = 0;

                    case STATE_SEND_ID_NAME:
                        sendKey = cred.idName[credPtr];
                        if(flagKeyCleared) {
                            buildReport(sendKey);
                        }

                        else {
                            if(clearKeyCnt == 10) {
                                clearKeyCnt = 0;
                                flagKeyCleared = 1;
                                buildReport(sendKey);
                            }

                            else {
                                buildReport(KEY_BS);
                                clearKeyCnt++;
                            }
                        }
                        state = STATE_RELEASE_ID_NAME;
                        break;

                    case STATE_RELEASE_ID_NAME:
                        // always send empty report when done sending a key
                        buildReport(0);

                        // upgrade credPtr only if we are not sending backspace key
                        if(flagKeyCleared)
                            credPtr++;

                        // if the next char is NULL
                        if(cred.idName[credPtr] == '\0') {
                            flagDone = 1;
                            credPtr = 0;
                            flagKeyCleared = 0;
                            state = STATE_WAIT;
                        }
                        // the next char is valid so we go back to state_short_key
                        else {
                            state = STATE_SEND_ID_NAME;
                        }
                        break;

                    case STATE_LONG_KEY:
                        credPtr = 0;
                        flagKeyCleared = 0;
                        state = STATE_SEND_ID_USERNAME;

                    case STATE_SEND_ID_USERNAME:
                        sendKey = cred.idUsername[credPtr];
                        // clear the previous idName
                        if(flagKeyCleared)
                            buildReport(sendKey);
                        else {
                            if(clearKeyCnt == 10) {
                                clearKeyCnt = 0;
                                flagKeyCleared = 1;
                                buildReport(sendKey);
                            }

                            else {
                                buildReport(KEY_BS);
                                clearKeyCnt++;
                            }
                        }

                        state = STATE_RELEASE_ID_USERNAME;
                        break;

                    case STATE_RELEASE_ID_USERNAME:
                        // always send empty report when done sending a key
                        buildReport(0);
                        if(flagKeyCleared)
                            credPtr++;

                        // if the next char is NULL
                        if(cred.idUsername[credPtr] == '\0') {
                            state = STATE_SEND_TAB;
                        }
                        // the next char is valid so we go back to state_short_key
                        else {
                            state = STATE_SEND_ID_USERNAME;
                        }
                        break;

                    case STATE_SEND_TAB:
                        // send tab character
                        buildReport(KEY_TAB);
                        state = STATE_RELEASE_TAB;
                        break;

                    case STATE_RELEASE_TAB:
                        buildReport(0);
                        credPtr = 0;
                        state = STATE_SEND_ID_PASSWORD;
                        break;

                    case STATE_SEND_ID_PASSWORD:
                        sendKey = cred.idPassword[credPtr];
                        buildReport(sendKey);
                        state = STATE_RELEASE_ID_PASSWORD;
                        break;

                    case STATE_RELEASE_ID_PASSWORD:
                        // always send empty report when done sending a key
                        buildReport(0);
                        credPtr++;

                        // we are done injecting data
                        if(cred.idPassword[credPtr] == '\0') {
                            flagDone = 1;
                            state = STATE_WAIT;
                        }
                        // the next char is valid so we go back to state_short_key
                        else {
                            state = STATE_SEND_ID_PASSWORD;
                        }
                        break;

                    // should not happen
                    default:
                        state = STATE_WAIT;
                }

                usbSetInterrupt((void *)&keyboard_report, sizeof(keyboard_report));
                LED_TOGGLE();
            }
        }
    }
    return 0;
}
