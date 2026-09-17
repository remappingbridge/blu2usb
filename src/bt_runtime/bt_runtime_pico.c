#include "blu2usb/bt_runtime/bt_runtime.h"

#include <stddef.h>

#include "btstack.h"
#include "g05_hog_host.h"
#include "pico/cyw43_arch.h"

#define BLU2USB_BT_RUNTIME_MAX_SESSION_SETUPS 4u

static blu2usb_bt_runtime_session_setup_fn
    g_session_setups[BLU2USB_BT_RUNTIME_MAX_SESSION_SETUPS];
static size_t g_session_setup_count;
static bool g_started;

bool blu2usb_bt_runtime_register_session_setup(
    blu2usb_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;
    for (size_t i = 0u; i < g_session_setup_count; ++i)
        if (g_session_setups[i] == session_setup) return true;
    if (g_session_setup_count >= BLU2USB_BT_RUNTIME_MAX_SESSION_SETUPS)
        return false;
    g_session_setups[g_session_setup_count++] = session_setup;
    return true;
}

bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;

    blu2usb_bt_runtime_reset();

    /* Physically accepted runtime rule: initialize CYW43/BTstack on core 0
     * and let pico_cyw43_arch_threadsafe_background service the stack from its
     * low-priority async context. BLE and Classic adapters share this owner. */
    if (cyw43_arch_init() != 0) return false;

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();
    att_server_init(profile_data, NULL, NULL);

    session_setup();
    for (size_t i = 0u; i < g_session_setup_count; ++i)
        if (g_session_setups[i] != session_setup) g_session_setups[i]();

    hci_power_control(HCI_POWER_ON);
    g_started = true;
    return true;
}
