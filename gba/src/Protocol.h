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
#define AUDIO_CHUNK_SIZE 66    // (sizeof(gsm_frame) * 2)
#define AUDIO_CHUNK_PADDING 2  // (so every chunk it's exactly 17 packets)
#define AUDIO_SIZE_PACKETS 17  // -----------------------------^^
#define SPI_MODE 3
#define SPI_SLOW_FREQUENCY 1600000
#define SPI_FAST_FREQUENCY 2600000
#define SPI_DELAY_MICROSECONDS 4
#define TRANSFER_SYNC_PERIOD 8
#define COLORS_PER_PACKET (PACKET_SIZE / COLOR_SIZE)
#define PIXELS_PER_PACKET (PACKET_SIZE / PIXEL_SIZE)
#define MAX_PIXELS_SIZE (TOTAL_PIXELS / PIXELS_PER_PACKET)
#define AUDIO_PADDED_SIZE (AUDIO_CHUNK_SIZE + AUDIO_CHUNK_PADDING)

// INPUT
#define VIRTUAL_GAMEPAD_NAME "Linked GBA"

// DIFFS
#define TEMPORAL_DIFF_THRESHOLD 1500
#define TEMPORAL_DIFF_SIZE (TOTAL_PIXELS / 8)

// FILES
#define PALETTE_CACHE_FILENAME "palette.cache"

// COMMANDS
#define AUDIO_BIT_MASK 0x80000000
#define CMD_RESET 0x99887766
#define CMD_RPI_OFFSET 1
#define CMD_GBA_OFFSET 2
#define CMD_FRAME_START 0x12345610
#define CMD_AUDIO 0x12345620
#define CMD_PIXELS 0x12345630
#define CMD_FRAME_END 0x12345640
#define CMD_RECOVERY 0x98765490

#endif  // PROTOCOL_H
