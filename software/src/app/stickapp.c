/*
 * File: stickapp.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-29
 * License: GNU GPL v3 (see LICENSE)
 *
 * User application software for StickPass
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

/* libusb */
#include <usb.h>

#include "stickapp.h"

int main(int argc, char **argv) {

    // open syslog
    openlog("StickApp", LOG_PERROR, LOG_USER);

    usb_dev_handle *handle = NULL;
    int nBytes = 0;

    if(argc < 2) {
        printf("StickApp v1.0 -- A CLI for use with the StickPass password manager\n\n");
        printf("Usage:\n");
        printf("    -i, --init_device <masterKey>          Initialize device with specified unlock key\n");
        printf("    -u, --unlock_device <masterKey>        Unlock device\n");
        printf("    -s, --send <idName> <idUser> <idPass>  Send credential to device\n");
        printf("    -g, --generate                         Generate a complex password\n");
        printf("    -c, --clear                            Clear sensitive data from device\n");
        printf("    -b, --backup <file>                    Backup data from device to local file\n");
        printf("    -h, --help                             Show this help menu\n\n");
        printf("Arguments details\n");
        printf("    <masterKey> key/password to unlock the device\n");
        printf("    <idName>    nickname associated with credential\n");
        printf("    <idUser>    username associated with credential\n");
        printf("    <idPass>    password associated with credential\n");
        exit(1);
    }

    handle = usbOpenDevice(USB_VID, vendorName, USB_PID, productName);

    if(handle == NULL) {
        syslog(LOG_INFO, "Error! Could not find USB Device!");
        exit(-1);
    }

    else {
        syslog(LOG_INFO, "Successfully opened device: VID=%04x PID=%04x", USB_VID, USB_PID);
    }

    // unlock device
    if(!strcmp(argv[1], "--unlock_device") || !strcmp(argv[1], "-u")) {
        // check length of unlock key
        if(strlen(argv[2]) != 7) {
            syslog(LOG_INFO, "Error! unlock key must be 7 characters!");
            exit(-1);
        }
        char tmpBuffer[8];
        tmpBuffer[0] = STATE_UNLOCK_DEVICE;
        memcpy(&tmpBuffer[1], argv[2], 7);
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
            USB_UNLOCK_DEVICE, 0, 0, (char *)tmpBuffer, sizeof(tmpBuffer), 5000);
        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%s", nBytes, tmpBuffer);
    }

    // init device
    else if(!strcmp(argv[1], "--init_device") || !strcmp(argv[1], "-i")) {
        if(strlen(argv[2]) != 7) {
            syslog(LOG_INFO, "Error! unlock key must be 7 characters!");
            exit(-1);
        }
        char tmpBuffer[8];
        tmpBuffer[0] = STATE_INIT_DEVICE;
        memcpy(&tmpBuffer[1], argv[2], 7);
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
            USB_INIT_DEVICE, 0, 0, (char *)tmpBuffer, sizeof(tmpBuffer), 5000);
        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%s", nBytes, tmpBuffer);
    }

    // generate complex password
    else if(!strcmp(argv[1], "--generate") || !strcmp(argv[1], "-g")) {
        syslog(LOG_INFO, "Not implemented yet!");
    }

    // backup data from device
    else if(!strcmp(argv[1], "--backup") || !strcmp(argv[1], "-b")) {
        syslog(LOG_INFO, "Not implemented yet!");
    }

    // clear device
    else if(!strcmp(argv[1], "--clear") || !strcmp(argv[1], "-c")) {
        char decision;
        syslog(LOG_INFO, "About to erase whole EEPROM on device");
        printf("Are you sure you want to clear memory content of device? (Y / N)\n");
        scanf("%c", &decision);
        if(decision != 'y') {
            syslog(LOG_INFO, "Aborted");
            exit(-1);
        }
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
            USB_CLEAR_EEPROM, 0, 0, 0, 0, 5000);
        syslog(LOG_INFO, "EEPROM erased!");
    }

    // send credential to device
    else if(!strcmp(argv[1], "--send") || !strcmp(argv[1], "-s")) {
        if(strlen(argv[2]) > ID_NAME_LEN) {
            syslog(LOG_INFO, "Error! idName must be less or equal to 10 characters!");
            exit(-1);
        }
        if(strlen(argv[3]) > ID_USERNAME_LEN) {
            syslog(LOG_INFO, "Error! idUsername must be less or equal to 32 characters!");
            exit(-1);
        }
        if(strlen(argv[4]) > ID_PASSWORD_LEN) {
            syslog(LOG_INFO, "Error! idName must be less or equal to 21 characters!");
            exit(-1);
        }
        int i, flagDone, flagFull, bufPtr;
        char tmpBuffer[8];
        int state = STATE_ID_UPLOAD_INIT;
        char idName[ID_NAME_LEN];
        char idUsername[ID_USERNAME_LEN];
        char idPassword[ID_PASSWORD_LEN];
        memcpy(idName, argv[2], ID_NAME_LEN);
        memcpy(idUsername, argv[3], ID_USERNAME_LEN);
        memcpy(idPassword, argv[4], ID_PASSWORD_LEN);

        flagDone = 0;
        memset(tmpBuffer, 0, sizeof(tmpBuffer));
        syslog(LOG_INFO, "Ready to roll");


        while(!flagDone) {
            switch(state) {
                // send empty msg to signal that id upload has been initiated
                case STATE_ID_UPLOAD_INIT:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    tmpBuffer[0] = STATE_ID_UPLOAD_INIT;
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, sizeof(tmpBuffer), 5000);
                    state = STATE_ID_NAME_SEND;
                    syslog(LOG_INFO, "Sent signal to initiate id upload");
                    break;

                // send the idName
                case STATE_ID_NAME_SEND:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    flagFull = 0;
                    bufPtr = 0;

                    // send in chunks of 8 bytes
                    while(!flagFull) {
                        // store state in first byte of msg
                        tmpBuffer[0] = STATE_ID_NAME_SEND;

                        // fill rest of buffer
                        for(i = 1; idName[bufPtr] != '\0' && i < 8; i++) {
                            tmpBuffer[i] = idName[bufPtr];
                            bufPtr++;
                        }
                        if(idName[bufPtr] == '\0') {
                            flagFull = 1;
                        }

                        nBytes = usb_control_msg(handle,
                                     USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                     USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, i, 5000);
                        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%s", nBytes, tmpBuffer);
                    }
                    state = STATE_ID_NAME_DONE;
                    break;

                case STATE_ID_NAME_DONE:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    tmpBuffer[0] = STATE_ID_NAME_DONE;
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, sizeof(tmpBuffer), 5000);
                    state = STATE_ID_USERNAME_SEND;
                    syslog(LOG_INFO, "Sent signal for idName done");
                    break;

                case STATE_ID_USERNAME_SEND:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    flagFull = 0;
                    bufPtr = 0;

                    // send in chunks of 8 bytes
                    while(!flagFull) {
                        // store state in first byte of msg
                        tmpBuffer[0] = STATE_ID_USERNAME_SEND;

                        // fill rest of buffer
                        for(i = 1; idUsername[bufPtr] != '\0' && i < 8; i++) {
                            tmpBuffer[i] = idUsername[bufPtr];
                            bufPtr++;
                        }

                        if(idUsername[bufPtr] == '\0') {
                            syslog(LOG_DEBUG, "reached NULL!");
                            flagFull = 1;
                        }

                        syslog(LOG_INFO, "Preparing to send:%s", tmpBuffer);
                        nBytes = usb_control_msg(handle,
                                     USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                     USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, i, 5000);
                        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%s", nBytes, tmpBuffer);
                    }
                    state = STATE_ID_USERNAME_DONE;
                    break;

                case STATE_ID_USERNAME_DONE:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    tmpBuffer[0] = STATE_ID_USERNAME_DONE;
                    syslog(LOG_INFO, "Preparing to send:%s", tmpBuffer);
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, sizeof(tmpBuffer), 5000);
                    state = STATE_ID_PASS_SEND;
                    syslog(LOG_INFO, "Sent signal for idUsername done");
                    break;

                case STATE_ID_PASS_SEND:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    flagFull = 0;
                    bufPtr = 0;

                    // send in chunks of 8 bytes
                    while(!flagFull) {
                        // store state in first byte of msg
                        tmpBuffer[0] = STATE_ID_PASS_SEND;

                        // fill rest of buffer
                        for(i = 1; idPassword[bufPtr] != '\0' && i < 8; i++) {
                            tmpBuffer[i] = idPassword[bufPtr];
                            bufPtr++;
                        }

                        if(idPassword[bufPtr] == '\0') {
                            syslog(LOG_DEBUG, "reached NULL!");
                            flagFull = 1;
                        }

                        syslog(LOG_INFO, "Preparing to send:%s", tmpBuffer);
                        nBytes = usb_control_msg(handle,
                                     USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                     USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, i, 5000);
                        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%s", nBytes, tmpBuffer);
                    }
                    state = STATE_ID_PASS_DONE;
                    break;

                case STATE_ID_PASS_DONE:
                    memset(tmpBuffer, 0, sizeof(tmpBuffer));
                    tmpBuffer[0] = STATE_ID_PASS_DONE;
                    syslog(LOG_INFO, "Preparing to send:%s", tmpBuffer);
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             USB_ID_UPLOAD, 0, 0, (char *)tmpBuffer, sizeof(tmpBuffer), 5000);
                    flagDone = 1;
                    syslog(LOG_INFO, "Sent signal for idPassword done");
                    break;
            }
        }
    }

    // free usb handle
    usb_close(handle);

    // close syslog
    closelog();

    return 0;
}

int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    // make standard request GET_DESCRIPTOR, type string and given index
    // (e.g. dev->iProduct)
    rval = usb_control_msg(dev,
        USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
        USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid,
        buffer, sizeof(buffer), 1000);

    if(rval < 0) // error
        return rval;

    // rval should be bytes read, but buffer[0] contains the actual response size
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0]; // string is shorter than bytes read

    if(buffer[1] != USB_DT_STRING) // second byte is the data type
        return 0; // invalid return type

    // we're dealing with UTF-16LE here so actual chars is half of rval,
    // and index 0 doesn't count
    rval /= 2;

    /* lossy conversion to ISO Latin1 */
    for(i = 1; i < rval && i < buflen; i++) {
        if(buffer[2 * i + 1] == 0)
            buf[i-1] = buffer[2 * i];
        else
            buf[i-1] = '?'; /* outside of ISO Latin1 range */
    }
    buf[i-1] = 0;
    return i-1;
}

usb_dev_handle *usbOpenDevice(int vendor, char *vendorName, int product, char *productName) {
    struct usb_bus *bus;
    struct usb_device *dev;
    char devVendor[256], devProduct[256];

    usb_dev_handle * handle = NULL;

    usb_init();
    usb_find_busses();
    usb_find_devices();

    for(bus=usb_get_busses(); bus; bus=bus->next) {
        for(dev=bus->devices; dev; dev=dev->next) {
            if(dev->descriptor.idVendor != vendor ||
               dev->descriptor.idProduct != product)
                continue;

            /* we need to open the device in order to query strings */
            if(!(handle = usb_open(dev))) {
                fprintf(stderr, "Warning: cannot open USB device: %sn",
                        usb_strerror());
                continue;
            }

            /* get vendor name */
            if(usbGetDescriptorString(handle, dev->descriptor.iManufacturer,
                                      0x0409, devVendor, sizeof(devVendor)) < 0) {
                fprintf(stderr,
                        "Warning: cannot query manufacturer for device: %sn",
                        usb_strerror());
                usb_close(handle);
                continue;
            }

            /* get product name */
            if(usbGetDescriptorString(handle, dev->descriptor.iProduct,
                                      0x0409, devProduct, sizeof(devVendor)) < 0) {
                fprintf(stderr,
                        "Warning: cannot query product for device: %sn",
                        usb_strerror());
                usb_close(handle);
                continue;
            }

            if(strcmp(devVendor, vendorName) == 0 &&
               strcmp(devProduct, productName) == 0)
                return handle;
            else
                usb_close(handle);
        }
    }
    return NULL;
}
