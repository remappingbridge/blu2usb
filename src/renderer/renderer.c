#include "blu2usb/renderer/renderer.h"

#include <string.h>

static const uint8_t k_alpha[26][7] = {
    {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},{0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
    {0x0f,0x10,0x10,0x10,0x10,0x10,0x0f},{0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},{0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
    {0x0f,0x10,0x10,0x17,0x11,0x11,0x0f},{0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
    {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f},{0x07,0x02,0x02,0x02,0x12,0x12,0x0c},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
    {0x11,0x1b,0x15,0x15,0x11,0x11,0x11},{0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},{0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
    {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d},{0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
    {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e},{0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0e},{0x11,0x11,0x11,0x11,0x11,0x0a,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0a},{0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
    {0x11,0x11,0x0a,0x04,0x04,0x04,0x04},{0x1f,0x01,0x02,0x04,0x08,0x10,0x1f},
};
static const uint8_t k_digit[10][7] = {
    {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},{0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
    {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f},{0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e},
    {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},{0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e},
    {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e},{0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},{0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e},
};

static uint8_t glyph_row(char character, uint8_t row)
{
    if (row >= 7u) return 0u;
    if (character >= 'A' && character <= 'Z') return k_alpha[(uint8_t)(character - 'A')][row];
    if (character >= '0' && character <= '9') return k_digit[(uint8_t)(character - '0')][row];
    switch (character) {
    case '-': return row == 3u ? 0x1fu : 0u;
    case ':': return (row == 2u || row == 5u) ? 0x04u : 0u;
    case '.': return row == 6u ? 0x04u : 0u;
    case '=': return (row == 2u || row == 4u) ? 0x1fu : 0u;
    case '/': return row <= 4u ? (uint8_t)(1u << (4u - row)) : 0u;
    case '\\': return row <= 4u ? (uint8_t)(1u << row) : 0u;
    case '>': { static const uint8_t rows[7]={0x10,0x08,0x04,0x02,0x04,0x08,0x10}; return rows[row]; }
    case '<': { static const uint8_t rows[7]={0x01,0x02,0x04,0x08,0x04,0x02,0x01}; return rows[row]; }
    case '?': { static const uint8_t rows[7]={0x0e,0x11,0x01,0x02,0x04,0x00,0x04}; return rows[row]; }
    case '!': return (row < 5u || row == 6u) ? 0x04u : 0u;
    default: return 0u;
    }
}

void blu2usb_ui_frame_reset(blu2usb_ui_frame_t *frame, bool learn_background, uint8_t hint_start_row)
{
    if (frame == NULL) return;
    if (hint_start_row > BLU2USB_RENDERER_TEXT_ROWS) hint_start_row = BLU2USB_RENDERER_TEXT_ROWS;
    frame->learn_background = learn_background;
    frame->hint_start_row = hint_start_row;
    for (size_t row = 0; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
        for (size_t column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
            frame->cells[row][column].character = ' ';
            frame->cells[row][column].tone = BLU2USB_UI_TONE_ACTIONABLE;
        }
    }
}

bool blu2usb_ui_frame_set_text(blu2usb_ui_frame_t *frame, uint8_t row, uint8_t column, const char *text, blu2usb_ui_tone_t tone)
{
    if (frame == NULL || text == NULL || row >= BLU2USB_RENDERER_TEXT_ROWS || column >= BLU2USB_RENDERER_TEXT_COLS) return false;
    size_t index = 0;
    while (text[index] != '\0' && (size_t)column + index < BLU2USB_RENDERER_TEXT_COLS) {
        frame->cells[row][(size_t)column + index].character = text[index];
        frame->cells[row][(size_t)column + index].tone = tone;
        ++index;
    }
    return text[index] == '\0';
}

bool blu2usb_ui_frame_set_tone_span(blu2usb_ui_frame_t *frame, uint8_t row, uint8_t column, uint8_t length, blu2usb_ui_tone_t tone)
{
    if (frame == NULL || row >= BLU2USB_RENDERER_TEXT_ROWS || column >= BLU2USB_RENDERER_TEXT_COLS || (size_t)column + length > BLU2USB_RENDERER_TEXT_COLS) return false;
    for (size_t index = 0; index < length; ++index) frame->cells[row][(size_t)column + index].tone = tone;
    return true;
}

static bool starts_with(const char *text, const char *prefix)
{
    return text != NULL && prefix != NULL && strncmp(text, prefix, strlen(prefix)) == 0;
}

static uint8_t first_hint_row(const blu2usb_screen_template_t *screen)
{
    for (uint8_t row = 1; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
        const char *text = screen->rows[row];
        if (starts_with(text, "JOY ") || starts_with(text, "KEY ") || starts_with(text, "ANY KEY")) return row;
    }
    return BLU2USB_RENDERER_TEXT_ROWS;
}

static int selected_row(const blu2usb_ux_model_t *ux)
{
    switch (ux->screen) {
    case BLU2USB_SCREEN_HOME:
    case BLU2USB_SCREEN_MOUSE_OPTIONS:
    case BLU2USB_SCREEN_OTHER_OPTIONS:
    case BLU2USB_SCREEN_EDIT_CUSTOM:
    case BLU2USB_SCREEN_LEFT_WILL_BECOME:
    case BLU2USB_SCREEN_RIGHT_WILL_BECOME:
    case BLU2USB_SCREEN_MIDDLE_WILL_BECOME:
    case BLU2USB_SCREEN_FORWARD_WILL_BECOME:
    case BLU2USB_SCREEN_BACKWARD_WILL_BECOME:
    case BLU2USB_SCREEN_SAVED_DEVICES:
        return (int)(1u + ux->selection);
    case BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE: return 5;
    case BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD:
    case BLU2USB_SCREEN_DEVICE_DETAILS_COMPOSITE: return 4;
    default: return -1;
    }
}

static void emphasize_row(blu2usb_ui_frame_t *frame, uint8_t row)
{
    for (uint8_t column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        if (frame->cells[row][column].character != ' ') frame->cells[row][column].tone = BLU2USB_UI_TONE_EMPHASIZED;
    }
}

static bool row_has_control(const char *row, blu2usb_control_t control)
{
    if (row == NULL) return false;
    switch (control) {
    case BLU2USB_CONTROL_JOY_UP: return strstr(row, "JOY UP") != NULL;
    case BLU2USB_CONTROL_JOY_DOWN: return strstr(row, "JOY DOWN") != NULL;
    case BLU2USB_CONTROL_JOY_LEFT: return strstr(row, "JOY LEFT") != NULL || strstr(row, "RIGHT\\LEFT") != NULL;
    case BLU2USB_CONTROL_JOY_RIGHT: return strstr(row, "JOY RIGHT") != NULL;
    case BLU2USB_CONTROL_JOY_PRESS: return strstr(row, "JOY PRESS") != NULL;
    case BLU2USB_CONTROL_KEY_A: return strstr(row, "KEY A") != NULL;
    case BLU2USB_CONTROL_KEY_B: return strstr(row, "KEY B") != NULL;
    case BLU2USB_CONTROL_KEY_X: return strstr(row, "KEY X") != NULL;
    case BLU2USB_CONTROL_KEY_Y: return strstr(row, "KEY Y") != NULL;
    default: return false;
    }
}

static void project_learn_pressed(const blu2usb_ux_model_t *ux, blu2usb_ui_frame_t *frame)
{
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_JOY_UP))
        (void)blu2usb_ui_frame_set_tone_span(frame,1,6,6,BLU2USB_UI_TONE_EMPHASIZED);
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_JOY_LEFT)) {
        (void)blu2usb_ui_frame_set_tone_span(frame,2,0,3,BLU2USB_UI_TONE_EMPHASIZED);
        (void)blu2usb_ui_frame_set_tone_span(frame,3,0,4,BLU2USB_UI_TONE_EMPHASIZED);
    }
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_JOY_PRESS)) {
        (void)blu2usb_ui_frame_set_tone_span(frame,2,7,3,BLU2USB_UI_TONE_EMPHASIZED);
        (void)blu2usb_ui_frame_set_tone_span(frame,3,6,5,BLU2USB_UI_TONE_EMPHASIZED);
    }
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_JOY_RIGHT)) {
        (void)blu2usb_ui_frame_set_tone_span(frame,2,14,3,BLU2USB_UI_TONE_EMPHASIZED);
        (void)blu2usb_ui_frame_set_tone_span(frame,3,13,5,BLU2USB_UI_TONE_EMPHASIZED);
    }
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_JOY_DOWN))
        (void)blu2usb_ui_frame_set_tone_span(frame,4,5,8,BLU2USB_UI_TONE_EMPHASIZED);
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_KEY_A))
        (void)blu2usb_ui_frame_set_tone_span(frame,5,15,5,BLU2USB_UI_TONE_EMPHASIZED);
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_KEY_B))
        (void)blu2usb_ui_frame_set_tone_span(frame,6,15,5,BLU2USB_UI_TONE_EMPHASIZED);
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_KEY_X))
        (void)blu2usb_ui_frame_set_tone_span(frame,7,15,5,BLU2USB_UI_TONE_EMPHASIZED);
    if (blu2usb_interaction_is_pressed(&ux->interaction, BLU2USB_CONTROL_KEY_Y)) {
        (void)blu2usb_ui_frame_set_tone_span(frame,6,0,11,BLU2USB_UI_TONE_EMPHASIZED);
        (void)blu2usb_ui_frame_set_tone_span(frame,7,1,10,BLU2USB_UI_TONE_EMPHASIZED);
        (void)blu2usb_ui_frame_set_tone_span(frame,8,2,18,BLU2USB_UI_TONE_EMPHASIZED);
    }
}

void blu2usb_ui_project(const blu2usb_ux_model_t *ux, blu2usb_ui_frame_t *frame)
{
    if (ux == NULL || frame == NULL) return;
    const blu2usb_screen_template_t *screen = blu2usb_ux_screen_template(ux->screen);
    if (screen == NULL) return;
    const bool learn = ux->screen == BLU2USB_SCREEN_LEARN_KEYS;
    const uint8_t hint = learn ? BLU2USB_RENDERER_TEXT_ROWS : first_hint_row(screen);
    blu2usb_ui_frame_reset(frame, learn, hint);

    for (uint8_t row = 0; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
        const char *text = screen->rows[row] != NULL ? screen->rows[row] : "";
        blu2usb_ui_tone_t tone;
        if (row == 0) tone = BLU2USB_UI_TONE_TITLE;
        else if (learn || row >= hint || text[0] == ' ') tone = BLU2USB_UI_TONE_ACTIONABLE;
        else tone = BLU2USB_UI_TONE_STATIC;
        (void)blu2usb_ui_frame_set_text(frame,row,0,text,tone);
    }

    if (learn) {
        project_learn_pressed(ux,frame);
        return;
    }

    if (ux->screen >= BLU2USB_SCREEN_LEFT_WILL_BECOME && ux->screen <= BLU2USB_SCREEN_BACKWARD_WILL_BECOME) {
        const uint8_t current_row = (uint8_t)(1u + (unsigned)ux->custom_targets[ux->custom_source]);
        for (uint8_t c = 0; c < BLU2USB_RENDERER_TEXT_COLS; ++c)
            if (frame->cells[current_row][c].character != ' ') frame->cells[current_row][c].tone = BLU2USB_UI_TONE_CURRENT;
    }

    const int selected = selected_row(ux);
    if (selected >= 0 && selected < (int)BLU2USB_RENDERER_TEXT_ROWS) emphasize_row(frame,(uint8_t)selected);

    for (unsigned control = 0; control < BLU2USB_CONTROL_COUNT; ++control) {
        if (!blu2usb_interaction_is_pressed(&ux->interaction,(blu2usb_control_t)control)) continue;
        for (uint8_t row = hint; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
            if (row_has_control(screen->rows[row],(blu2usb_control_t)control)) emphasize_row(frame,row);
        }
    }
}

uint16_t blu2usb_renderer_tone_rgb565(blu2usb_ui_tone_t tone)
{
    switch (tone) {
    case BLU2USB_UI_TONE_TITLE: return BLU2USB_COLOR_MAGENTA;
    case BLU2USB_UI_TONE_STATIC: return BLU2USB_COLOR_OFF_WHITE_YELLOW;
    case BLU2USB_UI_TONE_ACTIONABLE: return BLU2USB_COLOR_LIGHT_GRAY;
    case BLU2USB_UI_TONE_EMPHASIZED: return BLU2USB_COLOR_WHITE;
    case BLU2USB_UI_TONE_CURRENT: return BLU2USB_COLOR_CYAN;
    default: return BLU2USB_COLOR_LIGHT_GRAY;
    }
}

uint16_t blu2usb_renderer_background_rgb565(const blu2usb_ui_frame_t *frame, uint8_t row)
{
    if (frame == NULL) return BLU2USB_COLOR_BLACK;
    if (frame->learn_background) return BLU2USB_COLOR_DARK_MAGENTA;
    return row >= frame->hint_start_row ? BLU2USB_COLOR_DARK_MAGENTA : BLU2USB_COLOR_BLACK;
}

uint16_t blu2usb_renderer_separator_boundary_y(const blu2usb_ui_frame_t *frame)
{
    if (frame == NULL || frame->learn_background || frame->hint_start_row == 0u || frame->hint_start_row >= BLU2USB_RENDERER_TEXT_ROWS) return BLU2USB_RENDERER_HEIGHT;
    const uint16_t separator_row = (uint16_t)(frame->hint_start_row - 1u);
    const uint16_t boundary = (uint16_t)(BLU2USB_RENDERER_TEXT_Y + separator_row * BLU2USB_RENDERER_LINE_ADVANCE + BLU2USB_RENDERER_LINE_ADVANCE / 2u);
    return boundary < BLU2USB_RENDERER_HEIGHT ? boundary : BLU2USB_RENDERER_HEIGHT;
}

static uint16_t relocated_cell_x(uint8_t column)
{
    return (uint16_t)(BLU2USB_RENDERER_TEXT_X + (uint16_t)column * BLU2USB_RENDERER_CHAR_ADVANCE);
}

static uint16_t relocated_cell_y(uint8_t row)
{
    return (uint16_t)(BLU2USB_RENDERER_TEXT_Y + (uint16_t)row * BLU2USB_RENDERER_LINE_ADVANCE);
}

static bool draw_cell(const blu2usb_display_hal_t *display, const blu2usb_ui_frame_t *frame, uint8_t row, uint8_t column)
{
    const blu2usb_ui_cell_t *cell = &frame->cells[row][column];
    if (cell->character == ' ') return true;
    uint16_t pixels[BLU2USB_RENDERER_GLYPH_WIDTH * BLU2USB_RENDERER_GLYPH_HEIGHT];
    const uint16_t foreground = blu2usb_renderer_tone_rgb565(cell->tone);
    const uint16_t background = blu2usb_renderer_background_rgb565(frame,row);
    size_t out = 0;
    for (uint8_t py = 0; py < BLU2USB_RENDERER_GLYPH_HEIGHT; ++py) {
        const uint8_t bits = glyph_row(cell->character,(uint8_t)(py / BLU2USB_RENDERER_GLYPH_SCALE));
        for (uint8_t px = 0; px < BLU2USB_RENDERER_GLYPH_WIDTH; ++px) {
            const uint8_t source_x = (uint8_t)(px / BLU2USB_RENDERER_GLYPH_SCALE);
            const bool on = (bits & (uint8_t)(1u << (4u - source_x))) != 0u;
            pixels[out++] = on ? foreground : background;
        }
    }
    const uint16_t x = relocated_cell_x(column);
    const uint16_t y = relocated_cell_y(row);
    return display->write_rgb565(display->context,x,y,BLU2USB_RENDERER_GLYPH_WIDTH,BLU2USB_RENDERER_GLYPH_HEIGHT,pixels);
}

bool blu2usb_renderer_render(const blu2usb_display_hal_t *display, const blu2usb_ui_frame_t *frame)
{
    if (display == NULL || frame == NULL || display->fill_rect == NULL || display->write_rgb565 == NULL) return false;
    if (frame->learn_background) {
        if (!display->fill_rect(display->context,0,0,BLU2USB_RENDERER_WIDTH,BLU2USB_RENDERER_HEIGHT,BLU2USB_COLOR_DARK_MAGENTA)) return false;
    } else {
        const uint16_t boundary = blu2usb_renderer_separator_boundary_y(frame);
        if (boundary > 0u && !display->fill_rect(display->context,0,0,BLU2USB_RENDERER_WIDTH,boundary,BLU2USB_COLOR_BLACK)) return false;
        if (boundary < BLU2USB_RENDERER_HEIGHT && !display->fill_rect(display->context,0,boundary,BLU2USB_RENDERER_WIDTH,(uint16_t)(BLU2USB_RENDERER_HEIGHT-boundary),BLU2USB_COLOR_DARK_MAGENTA)) return false;
    }
    for (uint8_t row = 0; row < BLU2USB_RENDERER_TEXT_ROWS; ++row)
        for (uint8_t column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
            if (!draw_cell(display,frame,row,column)) return false;
    return true;
}
