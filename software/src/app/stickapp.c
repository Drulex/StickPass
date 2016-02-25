/*
 * File: stickapp.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-29
 * License: GNU GPL v2 (see License.txt)
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
    char buffer[64];

    if(argc < 2) {
        printf("Usage:\n");
        printf("./stickapp led_on\n");
        printf("./stickapp led_off\n");
        printf("./stickapp id_upload\n");
        exit(1);
    }

    handle = usbOpenDevice(USB_VID, vendorName, USB_PID, productName);

    if(handle == NULL) {
        syslog(LOG_DEBUG, "Error! Could not find USB Device!");
        exit(1);
    }

    else {
        syslog(LOG_INFO, "Successfully opened device: VID=%04x PID=%04x", USB_VID, USB_PID);
    }

    if(strcmp(argv[1], "led_on") == 0) {
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
            USB_LED_ON, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    }

    else if(strcmp(argv[1], "led_off") == 0) {
        nBytes = usb_control_msg(handle,
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
            USB_LED_OFF, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    }

    else if(strcmp(argv[1], "id_upload") == 0) {
        int len, i, flagDone;
        char key;
        int state = STATE_ID_UPLOAD_INIT;
        char idName[10] = "someapp";
        char idUsername[32] = "testuser@testapp.com";
        char idPassword[22] = "password123test";

        flagDone = 0;

        while(!flagDone) {
            switch(state) {
                // send empty msg to signal that id upload has been initiated
                case STATE_ID_UPLOAD_INIT:
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             STATE_ID_UPLOAD_INIT, 0, 0, 0, 0, 5000);
                    state = STATE_ID_NAME_SEND;
                    syslog(LOG_INFO, "Sent signal to initiade id upload");
                    break;

                // send the idName
                case STATE_ID_NAME_SEND:
                    // send idName one byte at a time
                    for(i = 0; idName[i] != '\0'; i++) {
                        key = idName[i];
                        nBytes = usb_control_msg(handle,
                                 USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                 STATE_ID_NAME_SEND, 0, 0, (char *) &key, 1, 5000);
                        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%c", nBytes, key);
                    }
                    state = STATE_ID_NAME_DONE;
                    break;

                case STATE_ID_NAME_DONE:
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             STATE_ID_NAME_DONE, 0, 0, 0, 0, 5000);
                    state = STATE_ID_USERNAME_SEND;
                    syslog(LOG_INFO, "Sent signal for idName done");
                    break;

                case STATE_ID_USERNAME_SEND:
                    // send idUsername one byte at a time
                    for(i = 0; idUsername[i] != '\0'; i++) {
                        key = idUsername[i];
                        nBytes = usb_control_msg(handle,
                                 USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                 STATE_ID_USERNAME_SEND, 0, 0, (char *) &key, 1, 5000);
                        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%c", nBytes, key);
                    }
                    state = STATE_ID_USERNAME_DONE;
                    break;

                case STATE_ID_USERNAME_DONE:
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             STATE_ID_USERNAME_DONE, 0, 0, 0, 0, 5000);
                    state = STATE_ID_PASS_SEND;
                    syslog(LOG_INFO, "Sent signal for idUsername done");
                    break;

                case STATE_ID_PASS_SEND:
                    // send idPassword one byte at a time
                    for(i = 0; idPassword[i] != '\0'; i++) {
                        key = idPassword[i];
                        nBytes = usb_control_msg(handle,
                                 USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                 STATE_ID_PASS_SEND, 0, 0, (char *) &key, 1, 5000);
                        syslog(LOG_INFO, "Sent %d bytes to USB device.\nDATA=%c", nBytes, key);
                    }
                    state = STATE_ID_PASS_DONE;
                    break;

                case STATE_ID_PASS_DONE:
                    nBytes = usb_control_msg(handle,
                             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                             STATE_ID_PASS_DONE, 0, 0, 0, 0, 5000);
                    flagDone = 1;
                    syslog(LOG_INFO, "Sent signal for idPassword done");
                    break;
            }
        }
    }

    else if(strcmp(argv[1], "interrupt_in") == 0) {
        nBytes = usb_interrupt_read(handle,
        USB_ENDPOINT_IN | 1, buffer, sizeof(buffer), 5000);
        syslog(LOG_INFO, "Received %d bytes from USB device.\nDATA=%s", nBytes, buffer);
    }
    if(nBytes < 0)
        fprintf(stderr, "USB error: %s", usb_strerror());

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
