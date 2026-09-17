#include "blu2usb/bt_runtime/bt_runtime.h"

#include <stddef.h>
#include <stdint.h>

#include "btstack.h"
#include "g05_hog_host.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#define BLU2USB_BT_RUNTIME_MAX_SESSION_SETUPS 4u
#define BLU2USB_BT_CORE1_STACK_SIZE_BYTES (8u * 1024u)
#define BLU2USB_BT_CORE1_START_DELAY_MS 500u

static blu2usb_bt_runtime_session_setup_fn
    g_session_setups[BLU2USB_BT_RUNTIME_MAX_SESSION_SETUPS];
static size_t g_session_setup_count;
static blu2usb_bt_runtime_session_setup_fn g_primary_session_setup;
static bool g_started;

/*
 * The physically proven BKB-3G POC and both subsequent Classic integrations in
 * picow-mouse-remapper keep CYW43/BTstack on Core1 and execute BTstack's run
 * loop there.  The integrated BLE+Classic PICO-08 build additionally required
 * an explicit 8 KiB SRAM stack; using the linker-owned Core1 stack was not a
 * reliable execution envelope for the dual transport workload on RP2350.
 *
 * Keep this storage in normal SRAM and launch Core1 with it explicitly.  Core0
 * remains the USB/HAT/LCD/application owner and communicates with Bluetooth via
 * the existing atomic runtime queue and command mailboxes.
 */
static uint32_t g_bt_core1_stack[BLU2USB_BT_CORE1_STACK_SIZE_BYTES / sizeof(uint32_t)]
    __attribute__((aligned(16)));

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

static void blu2usb_bt_core1_main(void)
{
    /* Register Core1 as a flash-safe lockout victim as well.  Product profile
     * writes happen on Core0 while BTstack can persist LE/Classic credentials
     * from Core1, so either core must be able to make the other safe for XIP. */
    (void)flash_safe_execute_core_init();

    /* PICO-08 deferred the heavier dual-protocol Bluetooth startup until after
     * the first UI frame.  G07 starts this core only after LCD/HAT/USB init, and
     * retains the proven additional settling interval without blocking Core0. */
    sleep_ms(BLU2USB_BT_CORE1_START_DELAY_MS);

    if (cyw43_arch_init() != PICO_OK) {
        for (;;) tight_loop_contents();
    }

    /* The threadsafe-background architecture still supplies the Pico SDK async
     * context used by CYW43, but the BTstack run loop itself has a dedicated
     * Core1 owner, matching every physically proven Classic HID implementation
     * for this hardware.  Serialize setup against the background worker. */
    async_context_acquire_lock_blocking(cyw43_arch_async_context());

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();
    att_server_init(profile_data, NULL, NULL);

    g_primary_session_setup();
    for (size_t i = 0u; i < g_session_setup_count; ++i)
        if (g_session_setups[i] != g_primary_session_setup) g_session_setups[i]();

    hci_power_control(HCI_POWER_ON);
    async_context_release_lock(cyw43_arch_async_context());

    /* Do not return: this is the proven Classic/dual-Bluetooth execution model.
     * The async-context run loop polls/waits for work and services BTstack timers
     * and callbacks on the Bluetooth core instead of relying solely on Core0's
     * background interrupt execution. */
    btstack_run_loop_execute();

    cyw43_arch_deinit();
    (void)flash_safe_execute_core_deinit();
    for (;;) tight_loop_contents();
}

bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;

    blu2usb_bt_runtime_reset();
    g_primary_session_setup = session_setup;

    /* Core0 is a flash-safe lockout victim before Core1 starts.  This mirrors
     * the known-good multicore firmware and also protects later product-profile
     * writes now that Bluetooth owns the other core. */
    if (!flash_safe_execute_core_init()) return false;

    g_started = true;
    multicore_launch_core1_with_stack(
        blu2usb_bt_core1_main,
        g_bt_core1_stack,
        sizeof(g_bt_core1_stack));
    return true;
}
