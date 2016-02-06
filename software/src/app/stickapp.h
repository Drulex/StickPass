/*
 * File: stickapp.h
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-27
 * License: GNU GPL v2 (see License.txt)
 *
 */

#ifndef STICKAPP_H
#define STICKAPP_H

#define USB_LED_OFF 0
#define USB_LED_ON 1
#define USB_DATA_OUT 2

#define USB_VID 0x16c0
#define USB_PID 0x05dc

// constants
const char *vendorName = "alexandru@jora.ca";
const char *productName = "StickPass";

// prototypes
int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, char *buf, int buflen);
usb_dev_handle *usbOpenDevice(int vendor, char *vendorName, int product,  char *productName);

#endif
