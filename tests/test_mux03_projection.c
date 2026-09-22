#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blu2usb/renderer/renderer_v1.h"

#ifndef BLU2USB_SOURCE_DIR
#error BLU2USB_SOURCE_DIR must be defined
#endif

static int failures = 0;

#define CHECK(condition) do {     if (!(condition)) {         fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         ++failures;     } } while (0)

static void canonical_product(
    blu2usb_device_registry_t *registry,
    blu2usb_product_snapshot_t *product)
{
    blu2usb_device_registry_init(registry);
    CHECK(blu2usb_device_registry_add(
        registry, UINT64_C(1), "LIFT",
        BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP));
    CHECK(blu2usb_device_registry_add(
        registry, UINT64_C(2), "OFFICE MOUSE",
        BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP));
    CHECK(blu2usb_device_registry_set_authoritative(registry, UINT64_C(1)));
    CHECK(blu2usb_product_snapshot_build(registry, product));
}

static bool read_golden_row(
    FILE *file,
    const char *screen,
    unsigned row,
    char out[BLU2USB_RENDERER_TEXT_COLS + 1u])
{
    char line[256];
    if (fgets(line, sizeof(line), file) == NULL) return false;

    char prefix[96];
    (void)snprintf(prefix, sizeof(prefix), "%s|%u|", screen, row);
    const size_t prefix_len = strlen(prefix);
    if (strncmp(line, prefix, prefix_len) != 0) return false;

    char *text = line + prefix_len;
    char *newline = strchr(text, '\n');
    if (newline != NULL) *newline = '\0';

    (void)snprintf(out, BLU2USB_RENDERER_TEXT_COLS + 1u, "%s", text);
    return true;
}

static void test_30_screen_golden(void)
{
    char path[512];
    (void)snprintf(
        path, sizeof(path), "%s/tests/goldens/mux03/screens.txt",
        BLU2USB_SOURCE_DIR);

    FILE *file = fopen(path, "r");
    CHECK(file != NULL);
    if (file == NULL) return;

    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;
    blu2usb_ui_frame_t frame;

    canonical_product(&registry, &product);
    blu2usb_ui_v1_init(&ui);

    CHECK((unsigned)BLU2USB_UI_V1_SCREEN_COUNT == 30u);

    for (blu2usb_ui_v1_screen_t screen = BLU2USB_UI_V1_SEARCHING_FIRST;
         screen < BLU2USB_UI_V1_SCREEN_COUNT;
         screen = (blu2usb_ui_v1_screen_t)((unsigned)screen + 1u)) {
        ui.screen = screen;
        ui.selection = 0u;
        ui.page = (screen == BLU2USB_UI_V1_SAVED_DEVICES ||
                   screen == BLU2USB_UI_V1_REMOVE_THIS) ? 1u : 0u;
        ui.remove_target_id =
            screen == BLU2USB_UI_V1_REMOVE_THIS ? UINT64_C(2)
                                               : BLU2USB_MOUSE_ID_INVALID;
        ui.custom_dirty = false;

        blu2usb_ui_v1_project_frame(&ui, &product, &frame);

        for (unsigned row = 0u; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
            char expected[BLU2USB_RENDERER_TEXT_COLS + 1u];
            char actual[BLU2USB_RENDERER_TEXT_COLS + 1u];
            CHECK(read_golden_row(
                file, blu2usb_ui_v1_screen_name(screen), row, expected));
            CHECK(blu2usb_ui_v1_row_text(&frame, (uint8_t)row, actual));
            if (strcmp(expected, actual) != 0) {
                fprintf(stderr,
                    "GOLDEN %s row %u expected='%s' actual='%s'\n",
                    blu2usb_ui_v1_screen_name(screen), row,
                    expected, actual);
                ++failures;
            }
        }
    }

    CHECK(fgetc(file) == EOF);
    fclose(file);
}

static void assert_name(const char *input, const char *expected)
{
    char output[BLU2USB_RENDERER_TEXT_COLS + 1u];
    blu2usb_ui_v1_format_mouse_name(input, output);
    if (strcmp(output, expected) != 0) {
        fprintf(stderr, "NAME '%s' expected='%s' actual='%s'\n",
                input, expected, output);
        ++failures;
    }
}

static void test_name_rules(void)
{
    assert_name("LIFT", "LIFT MOUSE");
    assert_name("MOUSE GENERIC", "MOUSE GENERIC");
    assert_name("XPTO ULTRA 2714", "XPTO ULTRA 2714 MOUSE");
    assert_name("ABCDEFGHIJKLMNOP", "ABCDEFGHIJKLMNO MOUSE");
    assert_name("ABCDEFGHIJKLMNO MOUSE", "ABCDEFGHIJKLMNO");
    assert_name("   ", "UNKNOWN MOUSE");
    assert_name("lift mouse pro", "LIFT MOUSE PRO");
}

static void test_colors_and_backgrounds(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;
    blu2usb_ui_frame_t frame;

    canonical_product(&registry, &product);
    blu2usb_ui_v1_init(&ui);

    ui.screen = BLU2USB_UI_V1_HOME_CONNECTED;
    ui.selection = 3u;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[1][1].tone == BLU2USB_UI_TONE_ACTIONABLE);
    CHECK(frame.cells[2][1].tone == BLU2USB_UI_TONE_ACTIONABLE);
    CHECK(frame.cells[3][1].tone == BLU2USB_UI_TONE_ACTIONABLE);
    CHECK(frame.cells[4][1].tone == BLU2USB_UI_TONE_EMPHASIZED);

    ui.screen = BLU2USB_UI_V1_REMAPPER_OPTIONS;
    ui.selection = 0u;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[3][1].tone == BLU2USB_UI_TONE_CURRENT);
    ui.selection = 2u;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[3][1].tone == BLU2USB_UI_TONE_EMPHASIZED);

    ui.screen = BLU2USB_UI_V1_SAVED_DEVICES;
    ui.page = 0u;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[1][0].tone == BLU2USB_UI_TONE_CURRENT);
    ui.page = 1u;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[1][0].tone == BLU2USB_UI_TONE_STATIC);

    ui.screen = BLU2USB_UI_V1_SEARCHING_FIRST;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[1][0].tone == BLU2USB_UI_TONE_STATIC);
    CHECK(frame.cells[2][0].tone == BLU2USB_UI_TONE_STATIC);
    CHECK(blu2usb_renderer_background_rgb565(&frame, 8u) ==
          BLU2USB_COLOR_BLACK);

    ui.screen = BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(blu2usb_renderer_background_rgb565(&frame, 7u) ==
          BLU2USB_COLOR_BLACK);
    CHECK(blu2usb_renderer_background_rgb565(&frame, 8u) ==
          BLU2USB_COLOR_DARK_MAGENTA);
}

static void test_pressed_feedback_and_custom_confirmed_state(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;
    blu2usb_ui_frame_t frame;

    canonical_product(&registry, &product);
    blu2usb_ui_v1_init(&ui);

    ui.screen = BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED;
    (void)blu2usb_ui_v1_input(
        &ui, &product, BLU2USB_CONTROL_KEY_Y, true);
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(frame.cells[6][15].tone == BLU2USB_UI_TONE_EMPHASIZED);
    CHECK(frame.cells[8][1].tone == BLU2USB_UI_TONE_EMPHASIZED);
    (void)blu2usb_ui_v1_input(
        &ui, &product, BLU2USB_CONTROL_KEY_Y, false);

    CHECK(blu2usb_device_registry_set_profile(
        &registry, UINT64_C(1), BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP));
    CHECK(blu2usb_product_snapshot_build(&registry, &product));

    ui.screen = BLU2USB_UI_V1_CUSTOM_EDIT;
    ui.custom_dirty = false;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    char row[BLU2USB_RENDERER_TEXT_COLS + 1u];
    CHECK(blu2usb_ui_v1_row_text(&frame, 8u, row));
    CHECK(strcmp(row, "") == 0);

    ui.custom_dirty = true;
    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(blu2usb_ui_v1_row_text(&frame, 8u, row));
    CHECK(strcmp(row, "KEY A: APPLY CUSTOM") == 0);
}

static void test_remove_target_identity_survives_reorder(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;
    blu2usb_ui_frame_t frame;
    char row[BLU2USB_RENDERER_TEXT_COLS + 1u];

    canonical_product(&registry, &product);
    blu2usb_ui_v1_init(&ui);
    ui.screen = BLU2USB_UI_V1_REMOVE_THIS;
    ui.remove_target_id = UINT64_C(2);

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(2)));
    CHECK(blu2usb_product_snapshot_build(&registry, &product));
    CHECK(product.mice[0].id == UINT64_C(2));

    blu2usb_ui_v1_project_frame(&ui, &product, &frame);
    CHECK(blu2usb_ui_v1_row_text(&frame, 1u, row));
    CHECK(strcmp(row, "OFFICE MOUSE") == 0);
}

int main(void)
{
    test_30_screen_golden();
    test_name_rules();
    test_colors_and_backgrounds();
    test_pressed_feedback_and_custom_confirmed_state();
    test_remove_target_identity_survives_reorder();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-03 projection assertion(s) failed\n", failures);
        return 1;
    }

    puts("MUX-03 UI Layout 1.0 projection PASS");
    return 0;
}
