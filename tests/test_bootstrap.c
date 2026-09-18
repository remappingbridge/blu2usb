#include "blu2usb/domain/version.h"

#include <string.h>

int main(void)
{
    return strcmp(blu2usb_version(), "0.7.0-g07-validated") == 0 ? 0 : 1;
}
