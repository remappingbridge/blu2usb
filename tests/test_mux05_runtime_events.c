#include <stdio.h>
#include <string.h>

#include "blu2usb/ble_hogp/ble_hogp.h"

static int failures;

#define CHECK(x) do { if (!(x)) {     fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);     ++failures; } } while (0)

static void test_provisional_event(
    blu2usb_ble_hogp_message_type_t message_type,
    blu2usb_ble_hogp_event_type_t expected_type)
{
    blu2usb_bt_runtime_message_t message;
    memset(&message, 0, sizeof(message));
    message.channel = BLU2USB_BLE_HOGP_RUNTIME_CHANNEL;
    message.type = (uint16_t)message_type;

    blu2usb_ble_hogp_provisional_event_t payload;
    memset(&payload, 0, sizeof(payload));
    payload.generation = UINT32_C(0x12345678);
    payload.peer.address_type = 1u;
    const uint8_t address[6] = {1u,2u,3u,4u,5u,6u};
    memcpy(payload.peer.address, address, sizeof(address));

    message.length = (uint16_t)sizeof(payload);
    memcpy(message.payload, &payload, sizeof(payload));

    blu2usb_ble_hogp_event_t event;
    CHECK(blu2usb_ble_hogp_decode_runtime_message(&message, &event));
    CHECK(event.type == expected_type);
    CHECK(event.generation == payload.generation);
    CHECK(event.peer.address_type == payload.peer.address_type);
    CHECK(memcmp(event.peer.address, address, sizeof(address)) == 0);

    message.length = (uint16_t)(sizeof(payload) - 1u);
    CHECK(!blu2usb_ble_hogp_decode_runtime_message(&message, &event));
}

static void test_legacy_runtime_messages_remain_compatible(void)
{
    blu2usb_bt_runtime_message_t message;
    blu2usb_ble_hogp_event_t event;
    memset(&message, 0, sizeof(message));

    message.channel = BLU2USB_BLE_HOGP_RUNTIME_CHANNEL;
    message.type = BLU2USB_BLE_HOGP_MESSAGE_CONNECTED;
    CHECK(blu2usb_ble_hogp_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_BLE_HOGP_EVENT_CONNECTED);

    message.type = BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED;
    CHECK(blu2usb_ble_hogp_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_BLE_HOGP_EVENT_DISCONNECTED);
}

int main(void)
{
    test_provisional_event(
        BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_READY,
        BLU2USB_BLE_HOGP_EVENT_PROVISIONAL_READY);
    test_provisional_event(
        BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
        BLU2USB_BLE_HOGP_EVENT_PROVISIONAL_CLEARED);
    test_provisional_event(
        BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_TIMEOUT,
        BLU2USB_BLE_HOGP_EVENT_PROVISIONAL_TIMEOUT);
    test_provisional_event(
        BLU2USB_BLE_HOGP_MESSAGE_PROMOTED,
        BLU2USB_BLE_HOGP_EVENT_PROMOTED);
    test_legacy_runtime_messages_remain_compatible();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-05 runtime event assertion(s) failed\n",
                failures);
        return 1;
    }

    puts("MUX-05 runtime event decode PASS");
    return 0;
}
