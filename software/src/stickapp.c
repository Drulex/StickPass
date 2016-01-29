#include <stdio.h>
#include "usb.h"

// Used to get descriptor string for device identification
int usbGetStringAscii(usb_dev_handle *dev, int index, char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    if((rval = usb_get_string_simple(dev, index, buf, buflen)) >= 0) /* use libusb version if it works */
        return rval;

    if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, 0x0409, buffer, sizeof(buffer), 5000)) < 0)
        return rval;

    if(buffer[1] != USB_DT_STRING){
        *buf = 0;
        return 0;
    }

    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1: */
    for(i=1;i<rval;i++){
        if(i > buflen)              /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }

    buf[i-1] = 0;
    return i-1;
}


