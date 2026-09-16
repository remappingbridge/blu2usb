#include "blu2usb/domain/version.h"
#include "pico/stdlib.h"

int main(void)
{
    (void)blu2usb_version();

    for (;;) {
        tight_loop_contents();
    }
}
