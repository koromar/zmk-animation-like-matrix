#include "matrix.h"
#include <zephyr/random/rand32.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(matrix, LOG_LEVEL_INF); // Optional: for logging

// Fixed point scaling factor (2^4 = 16)
#define FIXED_POINT_SCALE 16

// Helper to generate random number in a range
static uint32_t rand_range(uint32_t min, uint32_t max) {
    // Basic check to avoid modulo by zero or negative range
    if (max < min) {
        return min;
    }
    uint32_t range = max - min + 1;
    return sys_rand32_get() % range + min;
}

// Initialize a single drop
static void init_single_drop(matrix_drop_t *drop) {
    // For matrix style, drops usually start at the top and fall down
    // Keep X fixed for a column, randomize Y start position slightly above screen
    drop->x = rand_range(0, MATRIX_WIDTH - 1) * FIXED_POINT_SCALE;
    drop->y = - (rand_range(0, MATRIX_HEIGHT/2)) * FIXED_POINT_SCALE; // Start above screen
    drop->speed = rand_range(1 * FIXED_POINT_SCALE / 2, 2 * FIXED_POINT_SCALE / 2); // Vertical speed (0.5 to 1 pixels/frame)
    drop->bright = rand_range(1, 3); // Determines trail characteristics maybe?
    drop->length = rand_range(MATRIX_MIN_TRAIL_LENGTH, MATRIX_MAX_TRAIL_LENGTH);
}

// Draw the matrix effect onto the LVGL canvas
static void matrix_draw(matrix_anim_t *anim) {
    if (!anim || !anim->canvas || !anim->canvas_buf) {
        return;
    }

    lv_obj_t *canvas = anim->canvas;

    // 1. Fade existing pixels slightly (simulate decay)
    // This is a simple way; more complex might involve per-pixel alpha
    // For 1-bit, we might skip fading or just clear.
    // Let's clear for simplicity now, like the starfield.
    memset(anim->canvas_buf, 0, sizeof(lv_color_t) * MATRIX_WIDTH * MATRIX_HEIGHT);


    // 2. Draw each drop and its trail
    for (int i = 0; i < MATRIX_MAX_DROPS; i++) {
        matrix_drop_t *drop = &anim->drops[i];

        // Convert fixed point to integer coordinates
        int16_t x_int = drop->x / FIXED_POINT_SCALE;
        int16_t y_int = drop->y / FIXED_POINT_SCALE;
        uint8_t len = drop->length;
        uint8_t bright = drop->bright; // Use brightness for trail color/style

        // Draw trail (vertical)
        for (uint8_t trail_idx = 0; trail_idx < len; trail_idx += 1) { // Trail points
            int16_t trail_y = y_int - trail_idx; // Trail goes upwards from head

            // Check bounds
            if (x_int >= 0 && x_int < MATRIX_WIDTH && trail_y >= 0 && trail_y < MATRIX_HEIGHT) {
                // Calculate fade (simple linear fade for trail)
                uint8_t fade_level = ((len - trail_idx) * 255) / len;
                lv_color_t color;
                if (fade_level > 170) { // Brighter part of trail (closer to head)
                   color = lv_color_white();
                } else if (fade_level > 85) { // Mid trail
                   color = lv_color_hex(0xAAAAAA); // Light gray
                } else { // Faintest part
                   color = lv_color_hex(0x555555); // Dark gray
                }

                // Draw trail pixel
                lv_canvas_set_px_color(canvas, x_int, trail_y, color);
                // Matrix trails are usually 1px wide
            }
        }

        // Draw the main drop head (brightest point)
        if (x_int >= 0 && x_int < MATRIX_WIDTH && y_int >= 0 && y_int < MATRIX_HEIGHT) {
            lv_color_t drop_color = lv_color_white(); // Head is brightest
            lv_canvas_set_px_color(canvas, x_int, y_int, drop_color);
        }
    }

    // Notify LVGL that the canvas needs redraw (if buffer wasn't obtained via lv_canvas_get_buf)
    // lv_obj_invalidate(canvas);
}

// Update drop positions and reset if needed
static void matrix_update_positions(matrix_anim_t *anim) {
    for (int i = 0; i < MATRIX_MAX_DROPS; i++) {
        matrix_drop_t *drop = &anim->drops[i];

        // Update position (move down)
        drop->y += drop->speed;

        // If drop is off-screen below, reset it
        // Check if the *top* of the trail is off screen
        if ((drop->y / FIXED_POINT_SCALE - drop->length) > MATRIX_HEIGHT) {
            init_single_drop(drop);
        }
    }
}

// Work handler function called periodically
static void matrix_animation_work_handler(struct k_work *work) {
    matrix_anim_t *anim = CONTAINER_OF(work, matrix_anim_t, animation_work);

    if (!anim->active || !anim->canvas) {
        return;
    }

    matrix_update_positions(anim);
    matrix_draw(anim);

    // Reschedule the work item
    k_work_reschedule(&anim->animation_work, K_MSEC(MATRIX_FRAME_DELAY_MS));
}

// Public functions

void matrix_init(matrix_anim_t *anim, lv_obj_t *canvas) {
    if (!anim || !canvas) {
        LOG_ERR("Invalid arguments for matrix_init");
        return;
    }

    anim->canvas = canvas;
    anim->active = false;

    // Check if canvas has a buffer assigned
    anim->canvas_buf = lv_canvas_get_buf(canvas);
    if (!anim->canvas_buf) {
        LOG_ERR("Canvas buffer not available. Allocate it before calling matrix_init.");
        // Example allocation (ensure size matches MATRIX_WIDTH * MATRIX_HEIGHT):
        // static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(MATRIX_WIDTH, MATRIX_HEIGHT)];
        // lv_canvas_set_buffer(canvas, cbuf, MATRIX_WIDTH, MATRIX_HEIGHT, LV_IMG_CF_TRUE_COLOR);
        // anim->canvas_buf = cbuf;
        // If this ^ is done, no need for memset in matrix_draw
        return;
    }

    // Initialize all drops
    for (int i = 0; i < MATRIX_MAX_DROPS; i++) {
        init_single_drop(&anim->drops[i]);
    }

    // Initialize the work item
    k_work_init_delayable(&anim->animation_work, matrix_animation_work_handler);

    LOG_INF("Matrix animation initialized.");
}

void matrix_start(matrix_anim_t *anim) {
    if (!anim || !anim->canvas) {
        return;
    }
    if (anim->active) {
        return; // Already running
    }
    anim->active = true;
    // Start the animation loop immediately
    k_work_reschedule(&anim->animation_work, K_NO_WAIT);
    LOG_INF("Matrix animation started.");
}

void matrix_stop(matrix_anim_t *anim) {
    if (!anim) {
        return;
    }
    if (!anim->active) {
        return; // Already stopped
    }
    anim->active = false;
    // Cancel any pending work
    k_work_cancel_delayable(&anim->animation_work);
    LOG_INF("Matrix animation stopped.");
} 