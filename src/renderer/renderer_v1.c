#include "blu2usb/renderer/renderer_v1.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define EMPTY ""

typedef struct {
    const char *rows[BLU2USB_RENDERER_TEXT_ROWS];
} screen_rows_t;

static const screen_rows_t k_screens[BLU2USB_UI_V1_SCREEN_COUNT] = {
    [BLU2USB_UI_V1_SEARCHING_FIRST] = {{
        "SEARCHING FIRST MOUSE",
        "PRESS TO LEARN KEYS",
        "WHILE WAIT CONNECTION",
        "       JOY UP",
        "  JOY    JOY    JOY",
        "  LEFT  PRESS  RIGHT",
        "      JOY DOWN",
        " KEY A         KEY X",
        " KEY B         KEY Y",
    }},
    [BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED] = {{
        "FIRST MOUSE CONNECTED",
        "       JOY UP",
        "  JOY    JOY    JOY",
        "  LEFT  PRESS  RIGHT",
        "      JOY DOWN",
        " KEY A         KEY X",
        " KEY B         KEY Y",
        EMPTY,
        " KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_HOME_SEARCHING] = {{
        "SEARCHING SAVED MOUSE",
        " SAVED DEVICES",
        " PAIR NEW MOUSE",
        " LEARN THE KEYS",
        EMPTY,
        "KEY B: CANCEL SEARCH",
        "JOY UP / DOWN: SELECT",
        "JOY PRESS: ACCESS",
        "KEY X: HELP",
    }},
    [BLU2USB_UI_V1_HOME_SEARCHING_HELP] = {{
        "HOME SEARCHING HELP",
        "THE MATCHING ATTEMPT",
        "TOOK PLACE ONLY FOR",
        "DEVICES ALREADY SAVED",
        "IN THE PREFERENCES,",
        "BUT NOT FOR DEVICES",
        "THAT WERE NOT SAVED.",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_HOME_RETRY] = {{
        "DEVICE NOT FOUND",
        " SAVED DEVICES",
        " PAIR NEW MOUSE",
        " LEARN THE KEYS",
        EMPTY,
        "KEY A: RETRY SEARCH",
        "JOY UP / DOWN: SELECT",
        "JOY PRESS: ACCESS",
        "KEY X: HELP",
    }},
    [BLU2USB_UI_V1_HOME_RETRY_HELP] = {{
        "HOME RETRY HELP",
        "THE MATCHING ATTEMPT",
        "TOOK PLACE ONLY FOR",
        "DEVICES ALREADY SAVED",
        "IN THE PREFERENCES,",
        "BUT NOT FOR DEVICES",
        "THAT WERE NOT SAVED.",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_PAIR_NEW] = {{
        "PAIR NEW MOUSE",
        "TRYING TO CONNECT",
        "A NEW MOUSE THAT",
        "IS NOT LISTED",
        "IN SAVED DEVICES",
        EMPTY,
        "KEY B: CANCEL",
        "KEY X: HELP",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_HELP_PAIR_NEW] = {{
        "PAIR NEW DEVICE HELP",
        "TO CONNECT A SAVED",
        "DEVICE FIRST UNPLUG",
        "CURRENTLY CONNECTED",
        "MOUSE AND PRESS THE",
        "KEY B TO BACK UNTIL",
        "SEARCHING APPEARS.",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_RETRY_PAIR_NEW] = {{
        "PAIR NEW MOUSE",
        "NO NEW MOUSE OUTSIDE",
        "THE LIST OF SAVED",
        "DEVICES WAS FOUND",
        EMPTY,
        "KEY A: RETRY NEW PAIR",
        "KEY B: BACK TRY SAVED",
        "KEY X: HELP",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_HELP_RETRY_PAIR_NEW] = {{
        "DEVICE NOT FOUND HELP",
        "TO CONNECT A SAVED",
        "DEVICE FIRST UNPLUG",
        "CURRENTLY CONNECTED",
        "MOUSE AND PRESS THE",
        "KEY B TO BACK UNTIL",
        "SEARCHING APPEARS.",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_HOME_CONNECTED] = {{
        "MOUSE",
        " PASSTHROUGH",
        " SAVED DEVICES",
        " PAIR NEW MOUSE",
        " LEARN THE KEYS",
        EMPTY,
        "JOY UP / DOWN: SELECT",
        "JOY PRESS: ACCESS",
        "KEY X: HELP TO REMOVE",
    }},
    [BLU2USB_UI_V1_HELP_HOME_CONNECTED] = {{
        "REMOVE CONNECTED HELP",
        "TO DISCONNECT THE",
        "CURRENTLY CONNECTED",
        "MOUSE NAVIGATE TO:",
        "STEP 1. SAVED DEVICES",
        "STEP 2. REMOVE DEVICE",
        "STEP 3. KEY A: REMOVE",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_REMAPPER_OPTIONS] = {{
        "MOUSE OPTIONS",
        " PASSTHROUGH",
        " STANDARD REMAP",
        " ESCAPE REMAP",
        " CUSTOM REMAP",
        EMPTY,
        "JOY PRESS: ACCESS",
        "KEY B: BACK",
        "KEY X: HELP",
    }},
    [BLU2USB_UI_V1_HELP_REMAPPER_OPTIONS] = {{
        "REMAPPER OPTIONS HELP",
        "CHOOSE FROM THE",
        "OPTIONS TO CHANGE THE",
        "FUNCTIONS OF THE",
        "MOUSE BUTTONS.",
        "PASSTHROUGH IS THE",
        "DEFAULT OPTION.",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_PASSTHROUGH_ACTIVE] = {{
        "PASSTHROUGH ACTIVE",
        "ORIGINAL MOUSE",
        "BUTTONS POSITION",
        "ARE ACTIVE NOW",
        EMPTY,
        EMPTY,
        EMPTY,
        "KEY B: BACK",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE] = {{
        "APPLY PASSTHROUGH",
        "ORIGINAL MOUSE",
        "BUTTONS POSITION",
        "ARE NOT ACTIVE",
        EMPTY,
        EMPTY,
        "KEY A: APPLY",
        "KEY B: CANCEL",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_STANDARD_NOT_ACTIVE] = {{
        "APPLY STANDARD REMAP",
        "FORWARD IS LEFT",
        "LEFT IS FORWARD",
        "BACKWARD IS RIGHT",
        "RIGHT IS BACKWARD",
        EMPTY,
        "KEY A: APPLY",
        "KEY B: CANCEL",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_STANDARD_ACTIVE] = {{
        "STANDARD REMAP ACTIVE",
        "FORWARD IS LEFT",
        "LEFT IS FORWARD",
        "BACKWARD IS RIGHT",
        "RIGHT IS BACKWARD",
        EMPTY,
        EMPTY,
        "KEY B: BACK",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE] = {{
        "APPLY ESCAPE REMAP",
        "FORWARD IS LEFT",
        "BACKWARD IS RIGHT",
        "LEFT IS ESCAPE",
        "RIGHT IS BACKWARD",
        "MIDDLE IS FORWARD",
        EMPTY,
        "KEY A: APPLY",
        "KEY B: CANCEL",
    }},
    [BLU2USB_UI_V1_ESCAPE_ACTIVE] = {{
        "ESCAPE APPLIED ACTIVE",
        "FORWARD IS LEFT",
        "BACKWARD IS RIGHT",
        "LEFT IS ESCAPE",
        "RIGHT IS BACKWARD",
        "MIDDLE IS FORWARD",
        EMPTY,
        "KEY B: BACK",
        "KEY Y: LOCK",
    }},
    [BLU2USB_UI_V1_CUSTOM_EDIT] = {{
        "EDIT CUSTOM REMAP",
        " LEFT IS LEFT",
        " RIGHT IS RIGHT",
        " MIDDLE IS MIDDLE",
        " FORWARD IS FORWARD",
        " BACKWARD IS BACKWARD",
        EMPTY,
        "JOY PRESS: ACCESS",
        "KEY A: APPLY CUSTOM",
    }},
    [BLU2USB_UI_V1_LEFT] = {{
        "LEFT WILL BECOME",
        " LEFT",
        " RIGHT",
        " MIDDLE",
        " ESCAPE",
        " FORWARD",
        " BACKWARD",
        EMPTY,
        "KEY A: APPLY AND BACK",
    }},
    [BLU2USB_UI_V1_RIGHT] = {{
        "RIGHT WILL BECOME",
        " LEFT",
        " RIGHT",
        " MIDDLE",
        " ESCAPE",
        " FORWARD",
        " BACKWARD",
        EMPTY,
        "KEY A: APPLY AND BACK",
    }},
    [BLU2USB_UI_V1_MIDDLE] = {{
        "MIDDLE WILL BECOME",
        " LEFT",
        " RIGHT",
        " MIDDLE",
        " ESCAPE",
        " FORWARD",
        " BACKWARD",
        EMPTY,
        "KEY A: APPLY AND BACK",
    }},
    [BLU2USB_UI_V1_FORWARD] = {{
        "FORWARD WILL BECOME",
        " LEFT",
        " RIGHT",
        " MIDDLE",
        " ESCAPE",
        " FORWARD",
        " BACKWARD",
        EMPTY,
        "KEY A: APPLY AND BACK",
    }},
    [BLU2USB_UI_V1_BACKWARD] = {{
        "BACKWARD WILL BECOME",
        " LEFT",
        " RIGHT",
        " MIDDLE",
        " ESCAPE",
        " FORWARD",
        " BACKWARD",
        EMPTY,
        "KEY A: APPLY AND BACK",
    }},
    [BLU2USB_UI_V1_SAVED_DEVICES] = {{
        "1 OF 1",
        "MOUSE",
        "STATUS: DISCONNECTED",
        "PROFILE: PASSTHROUGH",
        " REMOVE DEVICE",
        EMPTY,
        "JOY RIGHT\\LEFT: PAGE",
        "JOY PRESS: ACCESS",
        "KEY B: BACK",
    }},
    [BLU2USB_UI_V1_REMOVE_THIS] = {{
        "REMOVE THIS MOUSE",
        "MOUSE",
        EMPTY,
        "PAIRING AND MAPPINGS",
        "WILL BE DELETED",
        EMPTY,
        "KEY A: REMOVE",
        "KEY B: CANCEL",
        "KEY X: HELP",
    }},
    [BLU2USB_UI_V1_HELP_REMOVE_THIS] = {{
        "REMOVE MOUSE HELP",
        "COMPLETELY REMOVE THE",
        "AUTOMATIC CONNECTION",
        "WHEN TURNING ON THE",
        "DEVICE AND DELETE ITS",
        "BUTTON REMAPPING",
        "PROFILE.",
        EMPTY,
        "ANY KEY: BACK",
    }},
    [BLU2USB_UI_V1_LEARN_THE_KEYS] = {{
        "PRESS TO LEARN KEYS",
        "       JOY UP",
        "  JOY    JOY    JOY",
        "  LEFT  PRESS  RIGHT",
        "      JOY DOWN",
        " KEY A         KEY X",
        " KEY B         KEY Y",
        EMPTY,
        " KEY Y: LOCK",
    }},
};

static bool ascii_word_char(char ch)
{
    const unsigned char value = (unsigned char)ch;
    return isalnum(value) != 0 || ch == '_';
}

static bool contains_mouse_word(const char *name)
{
    if (name == NULL) return false;
    const size_t length = strlen(name);

    for (size_t i = 0u; i + 5u <= length; ++i) {
        if (toupper((unsigned char)name[i + 0u]) == 'M' &&
            toupper((unsigned char)name[i + 1u]) == 'O' &&
            toupper((unsigned char)name[i + 2u]) == 'U' &&
            toupper((unsigned char)name[i + 3u]) == 'S' &&
            toupper((unsigned char)name[i + 4u]) == 'E') {
            const bool before = i > 0u && ascii_word_char(name[i - 1u]);
            const bool after = i + 5u < length && ascii_word_char(name[i + 5u]);
            if (!before && !after) return true;
        }
    }
    return false;
}

static bool supported_name_char(char ch)
{
    const unsigned char value = (unsigned char)ch;
    if (isalnum(value) != 0 || ch == ' ') return true;
    switch (ch) {
    case '-': case ':': case '.': case '=': case '/': case '\\':
    case '>': case '<': case '?': case '!':
        return true;
    default:
        return false;
    }
}

void blu2usb_ui_v1_format_mouse_name(
    const char *name,
    char out[BLU2USB_RENDERER_TEXT_COLS + 1u])
{
    if (out == NULL) return;
    out[0] = '\0';

    if (name == NULL) name = "";

    size_t output = 0u;
    for (size_t i = 0u; name[i] != '\0' && output < 15u; ++i) {
        if (!supported_name_char(name[i])) continue;
        out[output++] = (char)toupper((unsigned char)name[i]);
    }

    while (output > 0u && out[output - 1u] == ' ') --output;
    out[output] = '\0';

    if (output == 0u) {
        (void)snprintf(out, BLU2USB_RENDERER_TEXT_COLS + 1u, "UNKNOWN MOUSE");
        return;
    }

    if (!contains_mouse_word(name)) {
        const char suffix[] = " MOUSE";
        const size_t suffix_length = sizeof(suffix) - 1u;
        if (output + suffix_length <= BLU2USB_RENDERER_TEXT_COLS) {
            memcpy(&out[output], suffix, suffix_length + 1u);
        }
    }
}

static const char *profile_name(blu2usb_mouse_profile_kind_t profile)
{
    switch (profile) {
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH: return "PASSTHROUGH";
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP: return "STANDARD";
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP: return "ESCAPE";
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP: return "CUSTOM";
    default: return "PASSTHROUGH";
    }
}

static const char *target_name(blu2usb_mouse_target_t target)
{
    switch (target) {
    case BLU2USB_MOUSE_TARGET_LEFT: return "LEFT";
    case BLU2USB_MOUSE_TARGET_RIGHT: return "RIGHT";
    case BLU2USB_MOUSE_TARGET_MIDDLE: return "MIDDLE";
    case BLU2USB_MOUSE_TARGET_ESCAPE: return "ESCAPE";
    case BLU2USB_MOUSE_TARGET_FORWARD: return "FORWARD";
    case BLU2USB_MOUSE_TARGET_BACKWARD: return "BACKWARD";
    default: return "LEFT";
    }
}

static const blu2usb_mouse_snapshot_t *current_mouse(
    const blu2usb_product_snapshot_t *product)
{
    if (product == NULL || !product->has_authoritative) return NULL;
    return blu2usb_product_snapshot_find(product, product->authoritative_id);
}

static const blu2usb_mouse_snapshot_t *remove_target(
    const blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product)
{
    if (ui == NULL || product == NULL ||
        !blu2usb_mouse_id_is_valid(ui->remove_target_id))
        return NULL;
    return blu2usb_product_snapshot_find(product, ui->remove_target_id);
}

static uint8_t first_hint_row(blu2usb_ui_v1_screen_t screen)
{
    if (screen == BLU2USB_UI_V1_SEARCHING_FIRST)
        return BLU2USB_RENDERER_TEXT_ROWS;
    if (screen == BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED ||
        screen == BLU2USB_UI_V1_LEARN_THE_KEYS)
        return 8u;

    const screen_rows_t *rows = &k_screens[screen];
    for (uint8_t row = 1u; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
        const char *text = rows->rows[row];
        if (text == NULL) continue;
        if (strncmp(text, "KEY ", 4u) == 0 ||
            strncmp(text, "JOY ", 4u) == 0 ||
            strncmp(text, "ANY KEY", 7u) == 0)
            return row;
    }
    return BLU2USB_RENDERER_TEXT_ROWS;
}

static void set_row_tone(
    blu2usb_ui_frame_t *frame,
    uint8_t row,
    blu2usb_ui_tone_t tone)
{
    for (uint8_t column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
        if (frame->cells[row][column].character != ' ')
            frame->cells[row][column].tone = tone;
}

static int selected_row(const blu2usb_ui_v1_t *ui)
{
    switch (ui->screen) {
    case BLU2USB_UI_V1_HOME_SEARCHING:
    case BLU2USB_UI_V1_HOME_RETRY:
    case BLU2USB_UI_V1_HOME_CONNECTED:
    case BLU2USB_UI_V1_REMAPPER_OPTIONS:
    case BLU2USB_UI_V1_CUSTOM_EDIT:
    case BLU2USB_UI_V1_LEFT:
    case BLU2USB_UI_V1_RIGHT:
    case BLU2USB_UI_V1_MIDDLE:
    case BLU2USB_UI_V1_FORWARD:
    case BLU2USB_UI_V1_BACKWARD:
        return (int)(1u + ui->selection);
    default:
        return -1;
    }
}

static unsigned source_for_screen(blu2usb_ui_v1_screen_t screen)
{
    switch (screen) {
    case BLU2USB_UI_V1_LEFT: return BLU2USB_MOUSE_SOURCE_LEFT;
    case BLU2USB_UI_V1_RIGHT: return BLU2USB_MOUSE_SOURCE_RIGHT;
    case BLU2USB_UI_V1_MIDDLE: return BLU2USB_MOUSE_SOURCE_MIDDLE;
    case BLU2USB_UI_V1_FORWARD: return BLU2USB_MOUSE_SOURCE_FORWARD;
    case BLU2USB_UI_V1_BACKWARD: return BLU2USB_MOUSE_SOURCE_BACKWARD;
    default: return BLU2USB_MOUSE_SOURCE_COUNT;
    }
}

static unsigned target_visual_row(blu2usb_mouse_target_t target)
{
    switch (target) {
    case BLU2USB_MOUSE_TARGET_LEFT: return 1u;
    case BLU2USB_MOUSE_TARGET_RIGHT: return 2u;
    case BLU2USB_MOUSE_TARGET_MIDDLE: return 3u;
    case BLU2USB_MOUSE_TARGET_ESCAPE: return 4u;
    case BLU2USB_MOUSE_TARGET_FORWARD: return 5u;
    case BLU2USB_MOUSE_TARGET_BACKWARD: return 6u;
    default: return 1u;
    }
}

static bool row_has_control(const char *row, blu2usb_control_t control)
{
    if (row == NULL) return false;
    switch (control) {
    case BLU2USB_CONTROL_JOY_UP:
        return strstr(row, "JOY UP") != NULL || strstr(row, "UP / DOWN") != NULL;
    case BLU2USB_CONTROL_JOY_DOWN:
        return strstr(row, "JOY DOWN") != NULL || strstr(row, "UP / DOWN") != NULL;
    case BLU2USB_CONTROL_JOY_LEFT:
        return strstr(row, "LEFT: PAGE") != NULL || strstr(row, "RIGHT\\LEFT") != NULL;
    case BLU2USB_CONTROL_JOY_RIGHT:
        return strstr(row, "RIGHT") != NULL && strstr(row, "JOY") != NULL;
    case BLU2USB_CONTROL_JOY_PRESS:
        return strstr(row, "JOY PRESS") != NULL;
    case BLU2USB_CONTROL_KEY_A:
        return strstr(row, "KEY A") != NULL;
    case BLU2USB_CONTROL_KEY_B:
        return strstr(row, "KEY B") != NULL;
    case BLU2USB_CONTROL_KEY_X:
        return strstr(row, "KEY X") != NULL;
    case BLU2USB_CONTROL_KEY_Y:
        return strstr(row, "KEY Y") != NULL;
    default:
        return false;
    }
}

static void set_span(
    blu2usb_ui_frame_t *frame,
    uint8_t row,
    uint8_t column,
    uint8_t length)
{
    (void)blu2usb_ui_frame_set_tone_span(
        frame, row, column, length, BLU2USB_UI_TONE_EMPHASIZED);
}

static void project_didactic_pressed(
    const blu2usb_ui_v1_t *ui,
    blu2usb_ui_frame_t *frame,
    bool searching)
{
    const uint8_t up_row = searching ? 3u : 1u;
    const uint8_t joy_row = searching ? 4u : 2u;
    const uint8_t label_row = searching ? 5u : 3u;
    const uint8_t down_row = searching ? 6u : 4u;
    const uint8_t ax_row = searching ? 7u : 5u;
    const uint8_t by_row = searching ? 8u : 6u;

    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_JOY_UP))
        set_span(frame, up_row, 7u, 6u);
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_JOY_LEFT)) {
        set_span(frame, joy_row, 2u, 3u);
        set_span(frame, label_row, 2u, 4u);
    }
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_JOY_PRESS)) {
        set_span(frame, joy_row, 9u, 3u);
        set_span(frame, label_row, 8u, 5u);
    }
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_JOY_RIGHT)) {
        set_span(frame, joy_row, 16u, 3u);
        set_span(frame, label_row, 15u, 5u);
    }
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_JOY_DOWN))
        set_span(frame, down_row, 6u, 8u);
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_KEY_A))
        set_span(frame, ax_row, 1u, 5u);
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_KEY_X))
        set_span(frame, ax_row, 15u, 5u);
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_KEY_B))
        set_span(frame, by_row, 1u, 5u);
    if (blu2usb_interaction_is_pressed(&ui->interaction, BLU2USB_CONTROL_KEY_Y)) {
        set_span(frame, by_row, 15u, 5u);
        if (!searching)
            set_row_tone(frame, 8u, BLU2USB_UI_TONE_EMPHASIZED);
    }
}

static void dynamic_rows(
    const blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    const char *rows[BLU2USB_RENDERER_TEXT_ROWS],
    char storage[8][BLU2USB_RENDERER_TEXT_COLS + 1u])
{
    const blu2usb_mouse_snapshot_t *current = current_mouse(product);

    if (ui->screen == BLU2USB_UI_V1_HOME_CONNECTED && current != NULL) {
        blu2usb_ui_v1_format_mouse_name(current->name, storage[0]);
        rows[0] = storage[0];
        switch (current->profile) {
        case BLU2USB_MOUSE_PROFILE_PASSTHROUGH:
            rows[1] = " PASSTHROUGH"; break;
        case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP:
            rows[1] = " REMAPPED TO STANDARD"; break;
        case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP:
            rows[1] = " REMAPPED TO ESCAPE"; break;
        case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP:
            rows[1] = " REMAPPED TO CUSTOM"; break;
        default:
            rows[1] = " PASSTHROUGH"; break;
        }
    }

    if (ui->screen == BLU2USB_UI_V1_CUSTOM_EDIT) {
        static const char *const source_names[BLU2USB_MOUSE_SOURCE_COUNT] = {
            "LEFT", "RIGHT", "MIDDLE", "FORWARD", "BACKWARD"
        };
        const blu2usb_mouse_source_t sources[BLU2USB_MOUSE_SOURCE_COUNT] = {
            BLU2USB_MOUSE_SOURCE_LEFT,
            BLU2USB_MOUSE_SOURCE_RIGHT,
            BLU2USB_MOUSE_SOURCE_MIDDLE,
            BLU2USB_MOUSE_SOURCE_FORWARD,
            BLU2USB_MOUSE_SOURCE_BACKWARD,
        };
        for (unsigned i = 0u; i < BLU2USB_MOUSE_SOURCE_COUNT; ++i) {
            (void)snprintf(storage[i], BLU2USB_RENDERER_TEXT_COLS + 1u,
                " %s IS %s", source_names[i], target_name(ui->custom_targets[sources[i]]));
            rows[i + 1u] = storage[i];
        }
        if (current != NULL &&
            current->profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP &&
            !ui->custom_dirty)
            rows[8] = EMPTY;
    }

    if (ui->screen == BLU2USB_UI_V1_SAVED_DEVICES &&
        product != NULL && product->saved_count > 0u) {
        const size_t page = ui->page < product->saved_count ? ui->page : 0u;
        const blu2usb_mouse_snapshot_t *mouse = &product->mice[page];
        (void)snprintf(storage[0], sizeof(storage[0]), "%u OF %u",
            (unsigned)page + 1u, (unsigned)product->saved_count);
        rows[0] = storage[0];
        blu2usb_ui_v1_format_mouse_name(mouse->name, storage[1]);
        rows[1] = storage[1];
        rows[2] = mouse->connected ? "STATUS: CONNECTED" : "STATUS: DISCONNECTED";
        (void)snprintf(storage[2], sizeof(storage[2]), "PROFILE: %s",
            profile_name(mouse->profile));
        rows[3] = storage[2];
    }

    if (ui->screen == BLU2USB_UI_V1_REMOVE_THIS) {
        const blu2usb_mouse_snapshot_t *mouse = remove_target(ui, product);
        if (mouse != NULL) {
            blu2usb_ui_v1_format_mouse_name(mouse->name, storage[0]);
            rows[1] = storage[0];
        }
    }
}

void blu2usb_ui_v1_project_frame(
    const blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_ui_frame_t *frame)
{
    if (ui == NULL || frame == NULL ||
        (unsigned)ui->screen >= BLU2USB_UI_V1_SCREEN_COUNT)
        return;

    const uint8_t hint = first_hint_row(ui->screen);
    blu2usb_ui_frame_reset(frame, false, hint);

    const char *rows[BLU2USB_RENDERER_TEXT_ROWS];
    for (unsigned i = 0u; i < BLU2USB_RENDERER_TEXT_ROWS; ++i)
        rows[i] = k_screens[ui->screen].rows[i];

    char storage[8][BLU2USB_RENDERER_TEXT_COLS + 1u];
    memset(storage, 0, sizeof(storage));
    dynamic_rows(ui, product, rows, storage);

    for (uint8_t row = 0u; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
        const char *text = rows[row] == NULL ? EMPTY : rows[row];
        blu2usb_ui_tone_t tone;

        if (row == 0u) {
            tone = BLU2USB_UI_TONE_TITLE;
        } else if (row >= hint) {
            tone = BLU2USB_UI_TONE_ACTIONABLE;
        } else if (text[0] == ' ') {
            tone = BLU2USB_UI_TONE_ACTIONABLE;
        } else {
            tone = BLU2USB_UI_TONE_STATIC;
        }
        (void)blu2usb_ui_frame_set_text(frame, row, 0u, text, tone);
    }

    if (ui->screen == BLU2USB_UI_V1_SEARCHING_FIRST) {
        set_row_tone(frame, 1u, BLU2USB_UI_TONE_STATIC);
        set_row_tone(frame, 2u, BLU2USB_UI_TONE_STATIC);
        project_didactic_pressed(ui, frame, true);
        return;
    }

    if (ui->screen == BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED ||
        ui->screen == BLU2USB_UI_V1_LEARN_THE_KEYS) {
        project_didactic_pressed(ui, frame, false);
        return;
    }

    if (ui->screen == BLU2USB_UI_V1_PASSTHROUGH_ACTIVE) {
        for (uint8_t row = 1u; row <= 3u; ++row)
            set_row_tone(frame, row, BLU2USB_UI_TONE_CURRENT);
    } else if (ui->screen == BLU2USB_UI_V1_STANDARD_ACTIVE) {
        for (uint8_t row = 1u; row <= 4u; ++row)
            set_row_tone(frame, row, BLU2USB_UI_TONE_CURRENT);
    } else if (ui->screen == BLU2USB_UI_V1_ESCAPE_ACTIVE) {
        for (uint8_t row = 1u; row <= 5u; ++row)
            set_row_tone(frame, row, BLU2USB_UI_TONE_CURRENT);
    }

    if (ui->screen == BLU2USB_UI_V1_REMAPPER_OPTIONS) {
        const blu2usb_mouse_snapshot_t *mouse = current_mouse(product);
        if (mouse != NULL) {
            uint8_t row = 1u;
            switch (mouse->profile) {
            case BLU2USB_MOUSE_PROFILE_PASSTHROUGH: row = 1u; break;
            case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP: row = 2u; break;
            case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP: row = 3u; break;
            case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP: row = 4u; break;
            default: row = 1u; break;
            }
            set_row_tone(frame, row, BLU2USB_UI_TONE_CURRENT);
        }
    }

    if (ui->screen == BLU2USB_UI_V1_SAVED_DEVICES &&
        product != NULL && product->saved_count > 0u) {
        const size_t page = ui->page < product->saved_count ? ui->page : 0u;
        if (product->mice[page].connected)
            set_row_tone(frame, 1u, BLU2USB_UI_TONE_CURRENT);
    }

    const unsigned source = source_for_screen(ui->screen);
    if (source < BLU2USB_MOUSE_SOURCE_COUNT) {
        set_row_tone(frame,
            (uint8_t)target_visual_row(ui->custom_targets[source]),
            BLU2USB_UI_TONE_CURRENT);
    }

    const int selection = selected_row(ui);
    if (selection >= 0 && selection < (int)BLU2USB_RENDERER_TEXT_ROWS)
        set_row_tone(frame, (uint8_t)selection, BLU2USB_UI_TONE_EMPHASIZED);

    if (blu2usb_ui_v1_is_help(ui->screen) &&
        ui->interaction.held_mask != 0u) {
        set_row_tone(frame, 8u, BLU2USB_UI_TONE_EMPHASIZED);
        return;
    }

    for (unsigned control = 0u; control < BLU2USB_CONTROL_COUNT; ++control) {
        if (!blu2usb_interaction_is_pressed(
                &ui->interaction, (blu2usb_control_t)control))
            continue;
        for (uint8_t row = hint; row < BLU2USB_RENDERER_TEXT_ROWS; ++row)
            if (row_has_control(rows[row], (blu2usb_control_t)control))
                set_row_tone(frame, row, BLU2USB_UI_TONE_EMPHASIZED);
    }
}

bool blu2usb_ui_v1_row_text(
    const blu2usb_ui_frame_t *frame,
    uint8_t row,
    char out[BLU2USB_RENDERER_TEXT_COLS + 1u])
{
    if (frame == NULL || out == NULL || row >= BLU2USB_RENDERER_TEXT_ROWS)
        return false;

    size_t length = BLU2USB_RENDERER_TEXT_COLS;
    while (length > 0u && frame->cells[row][length - 1u].character == ' ')
        --length;

    for (size_t i = 0u; i < length; ++i)
        out[i] = frame->cells[row][i].character;
    out[length] = '\0';
    return true;
}
