#include "blu2usb/storage/storage.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico.h"

#define BLU2USB_STORAGE_SLOT_COUNT 2u

/* The pinned Pico SDK reserves two sectors for Bluetooth credentials. On
 * RP2350 it also leaves the final sector unused for the RP2350-E10 workaround.
 * Product state owns the two sectors immediately before that SDK-owned tail. */
#if PICO_RP2350 && PICO_RP2350_A2_SUPPORTED
#define BLU2USB_SDK_RESERVED_TAIL_SECTORS 3u
#else
#define BLU2USB_SDK_RESERVED_TAIL_SECTORS 2u
#endif

#define BLU2USB_PRODUCT_STORAGE_OFFSET     (PICO_FLASH_SIZE_BYTES -      ((BLU2USB_SDK_RESERVED_TAIL_SECTORS + BLU2USB_STORAGE_SLOT_COUNT) * FLASH_SECTOR_SIZE))

#if PICO_RP2040
#define BLU2USB_STORAGE_READ_BASE XIP_BASE
#else
#define BLU2USB_STORAGE_READ_BASE XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE
#endif

_Static_assert(BLU2USB_STORAGE_RECORD_SIZE <= FLASH_SECTOR_SIZE,
               "product record must fit one flash sector");
_Static_assert((BLU2USB_STORAGE_RECORD_SIZE % FLASH_PAGE_SIZE) == 0u,
               "product record must use whole flash pages");
_Static_assert((BLU2USB_PRODUCT_STORAGE_OFFSET % FLASH_SECTOR_SIZE) == 0u,
               "product storage must be sector aligned");

typedef struct {
    bool erase;
    uint32_t flash_offset;
    const uint8_t *data;
    size_t size;
} blu2usb_storage_mutation_t;

/* Keep large persistence workspaces out of the firmware stack. Storage is
 * synchronous and single-owner in the product composition. */
static uint8_t g_records[BLU2USB_STORAGE_SLOT_COUNT][BLU2USB_STORAGE_RECORD_SIZE];
static uint8_t g_encoded[BLU2USB_STORAGE_RECORD_SIZE];
static uint8_t g_scratch[BLU2USB_STORAGE_MAX_PAYLOAD_SIZE];

static uint32_t slot_offset(unsigned slot)
{
    return BLU2USB_PRODUCT_STORAGE_OFFSET + (uint32_t)slot * FLASH_SECTOR_SIZE;
}

static const uint8_t *slot_address(unsigned slot)
{
    return (const uint8_t *)(uintptr_t)(BLU2USB_STORAGE_READ_BASE + slot_offset(slot));
}

static bool storage_layout_is_safe(void)
{
    extern char __flash_binary_end;
    const uintptr_t binary_end =
        (uintptr_t)&__flash_binary_end - (uintptr_t)XIP_BASE;
    return binary_end <= BLU2USB_PRODUCT_STORAGE_OFFSET;
}

static void perform_mutation(void *context)
{
    const blu2usb_storage_mutation_t *mutation =
        (const blu2usb_storage_mutation_t *)context;

    if (mutation->erase) {
        flash_range_erase(mutation->flash_offset, FLASH_SECTOR_SIZE);
    } else {
        flash_range_program(
            mutation->flash_offset, mutation->data, mutation->size);
    }
}

static bool mutate(const blu2usb_storage_mutation_t *mutation)
{
    return flash_safe_execute(
               perform_mutation, (void *)mutation, UINT32_MAX) == PICO_OK;
}

static void read_record(
    unsigned slot,
    uint8_t record[BLU2USB_STORAGE_RECORD_SIZE])
{
    memcpy(record, slot_address(slot), BLU2USB_STORAGE_RECORD_SIZE);
}

static void read_all_records(void)
{
    read_record(0u, g_records[0]);
    read_record(1u, g_records[1]);
}

bool blu2usb_storage_load(
    uint8_t *payload,
    size_t payload_capacity,
    size_t *payload_size)
{
    if (!storage_layout_is_safe() || payload_size == NULL)
        return false;

    read_all_records();

    const int selected =
        blu2usb_storage_select_newest(g_records[0], g_records[1]);
    if (selected < 0)
        return false;

    return blu2usb_storage_record_decode(
        g_records[selected], NULL,
        payload, payload_capacity, payload_size);
}

bool blu2usb_storage_store(const uint8_t *payload, size_t payload_size)
{
    if (!storage_layout_is_safe() ||
        payload == NULL ||
        payload_size > BLU2USB_STORAGE_MAX_PAYLOAD_SIZE)
        return false;

    read_all_records();

    const int selected =
        blu2usb_storage_select_newest(g_records[0], g_records[1]);

    uint32_t generation = 0u;
    if (selected >= 0) {
        size_t scratch_size = 0u;
        if (!blu2usb_storage_record_decode(
                g_records[selected],
                &generation,
                g_scratch,
                sizeof(g_scratch),
                &scratch_size))
            return false;
    }

    generation += 1u;
    const unsigned target = selected == 0 ? 1u : 0u;

    if (!blu2usb_storage_record_encode(
            generation, payload, payload_size, g_encoded))
        return false;

    blu2usb_storage_mutation_t mutation = {
        .erase = true,
        .flash_offset = slot_offset(target),
        .data = NULL,
        .size = 0u,
    };
    if (!mutate(&mutation))
        return false;

    mutation.erase = false;
    mutation.data = g_encoded;
    mutation.size = BLU2USB_STORAGE_RECORD_SIZE;

    if (!mutate(&mutation))
        return false;

    read_record(target, g_records[target]);

    size_t verify_size = 0u;
    uint32_t verify_generation = 0u;

    return blu2usb_storage_record_decode(
               g_records[target],
               &verify_generation,
               g_scratch,
               sizeof(g_scratch),
               &verify_size) &&
           verify_generation == generation &&
           verify_size == payload_size &&
           memcmp(g_scratch, payload, payload_size) == 0;
}
