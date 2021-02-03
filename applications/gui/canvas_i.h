#pragma once

#include "canvas.h"
#include <u8g2.h>

struct Canvas {
    u8g2_t fb;
    uint8_t offset_x;
    uint8_t offset_y;
    uint8_t width;
    uint8_t height;
};

/*
 * Allocate memory and initialize canvas
 */
Canvas* canvas_init();

/*
 * Free canvas memory
 */
void canvas_free(Canvas* canvas);

/*
 * Reset canvas drawing tools configuration
 */
void canvas_reset(Canvas* canvas);

/*
 * Commit canvas. Send buffer to display
 */
void canvas_commit(Canvas* canvas);

/*
 * Set drawing region relative to real screen buffer
 */
void canvas_frame_set(
    Canvas* canvas,
    uint8_t offset_x,
    uint8_t offset_y,
    uint8_t width,
    uint8_t height);
