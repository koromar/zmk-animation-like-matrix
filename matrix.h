#pragma once

#include <zephyr/kernel.h>
#include <lvgl.h>

// Configuration (adjust as needed)
#define MATRIX_WIDTH 68   // New width for vertical orientation
#define MATRIX_HEIGHT 160 // New height for vertical orientation
#define MATRIX_MAX_DROPS 200 // Adjusted number of drops for larger area
#define MATRIX_FRAME_DELAY_MS 50 // Animation frame delay
#define MATRIX_MIN_TRAIL_LENGTH 5 // Slightly longer min trail for taller screen
#define MATRIX_MAX_TRAIL_LENGTH 40 // Slightly longer max trail for taller screen

// Structure to hold drop state (renamed from star)
typedef struct {
    int16_t x;        // X position (fixed point, 4 fractional bits)
    int16_t y;        // Y position (fixed point, 4 fractional bits)
    uint8_t speed;    // Movement speed (fixed point, 4 fractional bits) - repurposed for vertical speed
    uint8_t bright;   // Brightness (1-3, determines pixel width and trail fade)
    uint8_t length;   // Trail length (pixels)
} matrix_drop_t; // Renamed from starfield_star_t

// Structure to hold the animation state
typedef struct {
    lv_obj_t *canvas;           // LVGL canvas to draw on
    lv_color_t *canvas_buf;     // Buffer for the canvas
    matrix_drop_t drops[MATRIX_MAX_DROPS]; // Renamed from stars
    struct k_work_delayable animation_work; // Work item for scheduling updates
    bool active;                // Flag to control animation loop
} matrix_anim_t; // Renamed from starfield_anim_t

/**
 * @brief Initialize the matrix animation state.
 *
 * @param anim Pointer to the matrix_anim_t structure.
 * @param canvas Pointer to the LVGL canvas widget.
 */
void matrix_init(matrix_anim_t *anim, lv_obj_t *canvas);

/**
 * @brief Start the matrix animation updates.
 *
 * @param anim Pointer to the matrix_anim_t structure.
 */
void matrix_start(matrix_anim_t *anim);

/**
 * @brief Stop the matrix animation updates.
 *
 * @param anim Pointer to the matrix_anim_t structure.
 */
void matrix_stop(matrix_anim_t *anim); 