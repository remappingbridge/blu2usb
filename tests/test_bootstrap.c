#include "blu2usb/domain/version.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    assert(strcmp(blu2usb_version(), "0.1.0-g01") == 0);
    return 0;
}
