/* Run production bootstrap with observable platform seams; not hardware proof. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stddef.h>
#include "blu2usb/bt_runtime/bt_runtime.h"
static jmp_buf finished;
static void (*core)(void);
static unsigned phase, flash_inits;
void blu2usb_bt_runtime_reset(void) {assert(phase==0);phase=1;}
bool flash_safe_execute_core_init(void) {++flash_inits;return true;}
void multicore_launch_core1_with_stack(void (*fn)(void),uint32_t *stack,size_t bytes) {
    assert(phase==1 && flash_inits==1 && bytes==8192 && ((uintptr_t)stack%16)==0);core=fn;
}
void sleep_ms(uint32_t ms) {assert(ms==500 && flash_inits==2);phase=2;}
int cyw43_arch_init(void) {assert(phase==2);phase=3;return 0;}
void *cyw43_arch_async_context(void) {return &phase;}
void async_context_acquire_lock_blocking(void *p) {assert(p==&phase && phase==3);phase=4;}
void l2cap_init(void) {assert(phase==4);phase=5;}
void sm_init(void) {assert(phase==5);phase=6;}
void sm_set_io_capabilities(int n) {assert(phase==6 && n==3);}
void sm_set_authentication_requirements(int n) {assert(phase==6 && n==9);}
void gatt_client_init(void) {assert(phase==6);phase=7;}
void att_server_init(const unsigned char *p,void *a,void *b) {assert(phase==7 && p && !a && !b);phase=8;}
static void classic_setup(void) {assert(phase==8);phase=9;}
static void ble_setup(void) {assert(phase==9);phase=10;}
void hci_power_control(int n) {assert(phase==10 && n==1);phase=11;}
void async_context_release_lock(void *p) {assert(phase==11 && p==&phase);phase=12;}
void btstack_run_loop_execute(void) {assert(phase==12);phase=13;longjmp(finished,1);}
void tight_loop_contents(void) {assert(!"unexpected bootstrap failure");}
int main(void) {
    assert(!blu2usb_bt_runtime_start(NULL,classic_setup));
    assert(blu2usb_bt_runtime_start(ble_setup,classic_setup));
    assert(!blu2usb_bt_runtime_start(ble_setup,classic_setup));
    if(setjmp(finished)==0) core();
    assert(phase==13 && flash_inits==2);
}
