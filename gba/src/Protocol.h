#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Render modes:
// - Low quality (fast):
//   120x80, with DRAW_SCALE=2
// - High quality (slow):
//   240x160, with DRAW_SCALE=1
//   [!]                                                         [!]
//     When using this mode, move `compressedPixels` outside State
//     (from IWRAM to EWRAM). Otherwise, it'll crash.

// RENDER
#define RENDER_WIDTH 120
#define RENDER_HEIGHT 80
#define TOTAL_PIXELS (RENDER_WIDTH * RENDER_HEIGHT)
#define DRAW_SCALE 2
#define DRAW_WIDTH (RENDER_WIDTH * DRAW_SCALE)
#define DRAW_HEIGHT (RENDER_HEIGHT * DRAW_SCALE)
#define TOTAL_SCREEN_PIXELS (DRAW_WIDTH * DRAW_HEIGHT)
#define PALETTE_COLORS 256

// TRANSFER
#define PACKET_SIZE 4
#define COLOR_SIZE 2
#define PIXEL_SIZE 1
#define SPI_MODE 3
#define SPI_SLOW_FREQUENCY 1600000
#define SPI_FAST_FREQUENCY 2600000
#define SPI_DELAY_MICROSECONDS 4
#define MAX_BLIND_FRAMES 3
#define PRESSED_KEYS_MIN_VALIDATIONS 3
#define PRESSED_KEYS_REPETITIONS 10
#define COLORS_PER_PACKET (PACKET_SIZE / COLOR_SIZE)
#define PIXELS_PER_PACKET (PACKET_SIZE / PIXEL_SIZE)

// INPUT
#define VIRTUAL_GAMEPAD_NAME "Linked GBA"

// DIFFS
#define TEMPORAL_DIFF_THRESHOLD 1500
#define TEMPORAL_DIFF_SIZE (TOTAL_PIXELS / 8)

// FILES
#define PALETTE_CACHE_FILENAME "palette.cache"

// COMMANDS
#define CMD_RESET 0x98765400
#define CMD_RPI_OFFSET 1
#define CMD_GBA_OFFSET 2
#define CMD_FRAME_START 0x12345610
#define CMD_PIXELS_START 0x98765420
#define CMD_FRAME_END 0x98765430
#endif  // PROTOCOL_H
