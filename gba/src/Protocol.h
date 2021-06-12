#ifndef PROTOCOL_H
#define PROTOCOL_H

#define RENDER_WIDTH 120
#define RENDER_HEIGHT 80
#define TOTAL_PIXELS (RENDER_WIDTH * RENDER_HEIGHT)
#define DRAW_SCALE_X 2
#define DRAW_SCALE_Y 2
#define DRAW_WIDTH (RENDER_WIDTH * DRAW_SCALE_X)
#define DRAW_HEIGHT (RENDER_HEIGHT * DRAW_SCALE_Y)
#define TOTAL_SCREEN_PIXELS (DRAW_WIDTH * DRAW_HEIGHT)

#define PALETTE_COLORS 256
#define PACKET_SIZE 4
#define COLOR_SIZE 2
#define PIXEL_SIZE 1
#define SPI_MODE 3
#define SPI_SLOW_FREQUENCY 1600000
#define SPI_FAST_FREQUENCY 2600000
#define SPI_DELAY_MICROSECONDS 5
#define MAX_BLIND_FRAMES 3
#define DIFF_THRESHOLD 3000
#define SPATIAL_DIFF_BLOCK_SIZE 4
#define COLORS_PER_PACKET (PACKET_SIZE / COLOR_SIZE)
#define PIXELS_PER_PACKET (PACKET_SIZE / PIXEL_SIZE)
#define TEMPORAL_DIFF_SIZE (TOTAL_PIXELS / 8)
#define SPATIAL_DIFF_SIZE (TOTAL_PIXELS / SPATIAL_DIFF_BLOCK_SIZE / 8)
#define PRESSED_KEYS_MIN_VALIDATIONS 3
#define PRESSED_KEYS_REPETITIONS 10
#define VIRTUAL_GAMEPAD_NAME "Linked GBA"
#define PALETTE_CACHE_FILENAME "palette.cache"

#define CMD_RESET 0x98765400

#define CMD_FRAME_START_RPI 0x12345611
#define CMD_FRAME_START_GBA 0x12345612

#define CMD_PIXELS_START_RPI 0x98765421
#define CMD_PIXELS_START_GBA 0x98765422

#define CMD_FRAME_END_RPI 0x98765431
#define CMD_FRAME_END_GBA 0x98765432

#endif  // PROTOCOL_H
