#include "blu2usb/domain/version.h"

#include <string.h>

int main(void)
{
    return strcmp(blu2usb_version(), "0.6.2") == 0 ? 0 : 1;
}
