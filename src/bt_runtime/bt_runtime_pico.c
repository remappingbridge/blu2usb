#include "blu2usb/bt_runtime/bt_runtime.h"

#include "btstack.h"
#include "g05_hog_host.h"
#include "pico/cyw43_arch.h"

static blu2usb_bt_runtime_session_setup_fn g_session_setup;
static bool g_started;

bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;

    blu2usb_bt_runtime_reset();
    g_session_setup = session_setup;

    /* Physically accepted runtime rule: initialize CYW43/BTstack on core 0
     * and let pico_cyw43_arch_threadsafe_background service the stack from its
     * low-priority async context. This avoids the prior fragile Core1 owner. */
    if (cyw43_arch_init() != 0) {
        g_session_setup = NULL;
        return false;
    }

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();
    att_server_init(profile_data, NULL, NULL);

    g_session_setup();
    hci_power_control(HCI_POWER_ON);
    g_started = true;
    return true;
}
