#include "blu2usb/classic_hid/classic_hid.h"
#include "btstack.h"
#include <string.h>

// Reexpresses the parser physically accepted at POC b04aaf1. Framing and report
// IDs end here; downstream sees only logical usages/modifiers. See solution record.
bool blu2usb_classic_hid_parse(const uint8_t *descriptor, uint16_t descriptor_len,
    const uint8_t *report, uint16_t report_len, blu2usb_keyboard_snapshot_t *snapshot)
{
    if (!descriptor || !descriptor_len || !report || !snapshot ||
        report_len < 2u || report[0] != 0xa1u) return false;
    const bool ids = btstack_hid_report_id_declared(descriptor, descriptor_len);
    const uint16_t id = ids ? report[1] : HID_REPORT_ID_UNDEFINED;
    const int payload_size = btstack_hid_get_report_size_for_id(
        id, HID_REPORT_TYPE_INPUT, descriptor, descriptor_len);
    // Reject truncated input before the parser can read a missing field. Some
    // non-keyboard reports carry trailing bytes; they are ignored by usage page.
    if (payload_size <= 0 || (unsigned)payload_size + 1u + (ids ? 1u : 0u) > report_len)
        return false;
    blu2usb_keyboard_snapshot_t result = {0};
    bool keyboard_page = false;
    bool rollover = false;
    unsigned count = 0;
    btstack_hid_parser_t parser;
    btstack_hid_parser_init(&parser, descriptor, descriptor_len, HID_REPORT_TYPE_INPUT,
                            report + 1, report_len - 1u);
    while (btstack_hid_parser_has_more(&parser)) {
        uint16_t page, usage;
        int32_t value;
        btstack_hid_parser_get_field(&parser, &page, &usage, &value);
        if (page != 7u) continue;
        keyboard_page = true;
        if (!value || !usage) continue;
        if (usage >= 0xe0u && usage <= 0xe7u) {
            result.modifiers |= (uint8_t)(1u << (usage - 0xe0u));
        } else {
            bool duplicate = false;
            for (unsigned i = 0; i < count; ++i) duplicate |= result.keys[i] == usage;
            if (duplicate) continue;
            if (usage > 0xffu || count == 6u || usage <= 3u) rollover = true;
            else result.keys[count++] = (uint8_t)usage;
        }
    }
    if (!keyboard_page) return false;
    if (rollover) memset(result.keys, 1, sizeof(result.keys));
    *snapshot = result;
    return true;
}
