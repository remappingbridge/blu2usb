#include "blu2usb/domain/version.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    assert(strcmp(blu2usb_version(), "0.2.0-g02") == 0);
    return 0;
}
