#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zmk/display/widget.h>
#include "../matrix.h" // Include the animation logic header

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Define widget configuration (matches matrix.h)
#define WIDGET_WIDTH MATRIX_WIDTH
#define WIDGET_HEIGHT MATRIX_HEIGHT

// Structure to hold the widget's state
struct matrix_widget_state {
    lv_obj_t *canvas;
    // Adjust buffer size calculation if color format changes
    lv_color_t canvas_buf[LV_CANVAS_BUF_SIZE_INDEXED_1BIT(WIDGET_WIDTH, WIDGET_HEIGHT)]; // Buffer for 1-bit color
    matrix_anim_t anim_state; // Instance of the animation state
};

// Function to render the widget
static void matrix_widget_render(struct zmk_widget *widget, lv_obj_t *parent) {
    struct matrix_widget_state *state = widget->data;

    // Create the canvas if it doesn't exist
    if (state->canvas == NULL) {
        state->canvas = lv_canvas_create(parent);
        // Set buffer for 1-bit indexed color format
        lv_canvas_set_buffer(state->canvas, state->canvas_buf, WIDGET_WIDTH, WIDGET_HEIGHT, LV_IMG_CF_INDEXED_1BIT);
        // Set the palette for 1-bit color (0 = black, 1 = white/green)
        // Index 0 is transparent/background by default.
        // Index 1 will be the foreground color.
        // We could use green shades, but white is simple for monochrome.
        lv_canvas_set_palette(state->canvas, 0, lv_color_black());
        lv_canvas_set_palette(state->canvas, 1, lv_color_white()); // Or a green: lv_color_hex(0x00FF00)

        lv_obj_align(state->canvas, LV_ALIGN_TOP_LEFT, 0, 0); // Adjust alignment as needed

        // Initialize the animation logic, passing the canvas
        matrix_init(&state->anim_state, state->canvas);

        // Start the animation when the widget is first rendered
        matrix_start(&state->anim_state);
         LOG_INF("Matrix widget canvas created and animation started.");
    } else {
        // Ensure the canvas is still associated with the parent (LVGL might detach it)
         lv_obj_set_parent(state->canvas, parent);
    }

    // Note: The actual drawing happens in the matrix_animation_work_handler via matrix_draw
}

// Function called when the widget is removed or hidden (optional but good practice)
static void matrix_widget_cleanup(struct zmk_widget *widget) {
    struct matrix_widget_state *state = widget->data;
    if (state != NULL && state->canvas != NULL) {
        LOG_INF("Stopping matrix animation and cleaning up widget.");
        // Stop the animation
        matrix_stop(&state->anim_state);
        // Clean up LVGL object (optional, depends on LVGL screen management)
        // lv_obj_del(state->canvas);
        // state->canvas = NULL;
    }
}


// Widget initialization function
static int matrix_widget_init(struct zmk_widget *widget) {
    struct matrix_widget_state *state = widget->data;
    if (state == NULL) {
        LOG_ERR("Widget data (state) is NULL");
        return -EINVAL;
    }
    // Initial state setup
    state->canvas = NULL;
    // anim_state is initialized in matrix_widget_render when canvas is ready
    LOG_INF("Matrix widget initialized.");
    return 0;
}

// Define the widget structure (static instance)
static struct matrix_widget_state matrix_state_data;

struct zmk_widget zmk_widget_matrix = {
    .init = matrix_widget_init,
    .update = NULL, // Update is handled by the animation's work handler
    .render = matrix_widget_render,
    // Uncomment if ZMK supports cleanup callbacks in your version
    // .cleanup = matrix_widget_cleanup,
    .data = &matrix_state_data, // Point to the state data
};

// Function to get a pointer to the widget (used in keymap)
struct zmk_widget *get_zmk_widget_matrix(void) {
    return &zmk_widget_matrix;
} 