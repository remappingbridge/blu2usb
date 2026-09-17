#include "blu2usb/hid_aggregator/hid_aggregator.h"

#include <stdint.h>

static bool component_can_consume(int32_t pending, int32_t amount)
{
    if (amount == 0) return true;
    if (pending == 0) return false;
    if ((pending > 0) != (amount > 0)) return false;
    if (pending > 0) return amount <= pending;
    return amount >= pending;
}

bool blu2usb_hid_aggregator_consume_relative(blu2usb_hid_aggregator_t *aggregator,
                                              int32_t dx,
                                              int32_t dy,
                                              int32_t wheel_vertical,
                                              int32_t wheel_horizontal)
{
    if (aggregator == NULL) return false;
    if (!component_can_consume(aggregator->pending_dx, dx) ||
        !component_can_consume(aggregator->pending_dy, dy) ||
        !component_can_consume(aggregator->pending_wheel_vertical, wheel_vertical) ||
        !component_can_consume(aggregator->pending_wheel_horizontal, wheel_horizontal)) {
        return false;
    }

    aggregator->pending_dx -= dx;
    aggregator->pending_dy -= dy;
    aggregator->pending_wheel_vertical -= wheel_vertical;
    aggregator->pending_wheel_horizontal -= wheel_horizontal;
    return true;
}
