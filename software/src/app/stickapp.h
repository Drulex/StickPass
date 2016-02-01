#ifndef STICKAPP_H
#define STICKAPP_H

#define USB_LED_OFF 0
#define USB_LED_ON 1
#define USB_DATA_OUT 2

#define USB_VID 0x16c0
#define USB_PID 0x05dc

char *vendorName = "alexandru@jora.ca";
char *productName = "StickPass";

int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, char *buf, int buflen);
usb_dev_handle *usbOpenDevice(int vendor, char *vendorName, int product,  char *productName);

#endif
