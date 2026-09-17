#include "blu2usb/bt_runtime/bt_runtime.h"
#include "btstack.h"
#include "g05_hog_host.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

/* PICO-08 envelope (8fbb36f / 208a487), explicit two-adapter composition.
 * Core0 owns USB/HAT. Core1 owns exactly one CYW43/BTstack instance. */
static uint32_t g_core1_stack[8192 / sizeof(uint32_t)] __attribute__((aligned(16)));
static blu2usb_bt_runtime_session_setup_fn g_ble_setup, g_companion_setup;
static bool g_started;

static void bluetooth_core(void)
{
    if (!flash_safe_execute_core_init()) for (;;) tight_loop_contents();
    sleep_ms(500);
    if (cyw43_arch_init() != 0) for (;;) tight_loop_contents();
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    l2cap_init();
    sm_init();
    /* Keep G06 LE pairing policy. Classic SSP separately uses DISPLAY_ONLY. */
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();
    att_server_init(profile_data, NULL, NULL);
    if (g_companion_setup) g_companion_setup();
    g_ble_setup();
    hci_power_control(HCI_POWER_ON);
    async_context_release_lock(cyw43_arch_async_context());
    btstack_run_loop_execute();
    for (;;) tight_loop_contents();
}

bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn session_setup,
                             blu2usb_bt_runtime_session_setup_fn companion_setup)
{
    if (g_started || session_setup == NULL) return false;
    blu2usb_bt_runtime_reset();
    g_ble_setup = session_setup;
    g_companion_setup = companion_setup;
    if (!flash_safe_execute_core_init()) return false;
    g_started = true;
    multicore_launch_core1_with_stack(bluetooth_core, g_core1_stack, sizeof(g_core1_stack));
    return true;
}
