#include "blu2usb/domain/version.h"

#ifndef BLU2USB_VERSION_STRING
#define BLU2USB_VERSION_STRING "0.0.0-unknown"
#endif

const char *blu2usb_version(void)
{
    return BLU2USB_VERSION_STRING;
}
