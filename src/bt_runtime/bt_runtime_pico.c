#include "blu2usb/bt_runtime/bt_runtime.h"

#include "btstack.h"
#include "g05_hog_host.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include <stdatomic.h>

static blu2usb_bt_runtime_session_setup_fn g_session_setup;
static bool g_started;
static atomic_bool g_initialized = ATOMIC_VAR_INIT(false);
static atomic_bool g_failed = ATOMIC_VAR_INIT(false);
static uint32_t g_core1_stack[8192u / sizeof(uint32_t)] __attribute__((aligned(8)));

static void radio_core_main(void)
{
    // Both cores may initiate flash writes; keep the peer lockout handler live.
    if (!flash_safe_execute_core_init() || cyw43_arch_init() != 0) {
        atomic_store_explicit(&g_failed, true, memory_order_release);
        for (;;) tight_loop_contents();
    }
    async_context_acquire_lock_blocking(cyw43_arch_async_context());

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();
    att_server_init(profile_data, NULL, NULL);

    g_session_setup();
    atomic_store_explicit(&g_initialized, true, memory_order_release);
    hci_power_control(HCI_POWER_ON);
    async_context_release_lock(cyw43_arch_async_context());
    btstack_run_loop_execute();
    for (;;) tight_loop_contents();
}

bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn session_setup)
{
    if (g_started || session_setup == NULL) return false;
    blu2usb_bt_runtime_reset();
    g_session_setup = session_setup;
    if (!flash_safe_execute_core_init()) {
        atomic_store_explicit(&g_failed, true, memory_order_release);
        return false;
    }
    g_started = true;
    multicore_launch_core1_with_stack(radio_core_main, g_core1_stack, sizeof(g_core1_stack));
    return true;
}

bool blu2usb_bt_runtime_flash_lock(void)
{
    if (!atomic_load_explicit(&g_initialized, memory_order_acquire)) return false;
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    return true;
}

bool blu2usb_bt_runtime_failed(void)
{
    return atomic_load_explicit(&g_failed, memory_order_acquire);
}

void blu2usb_bt_runtime_flash_unlock(void)
{
    async_context_release_lock(cyw43_arch_async_context());
}
