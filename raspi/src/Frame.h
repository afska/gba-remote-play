#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t totalPixels;
  uint8_t* raw8BitPixels;
  uint16_t* raw15bppPalette;

  uint16_t getColorOf(uint32_t pixelId) {
    return raw15bppPalette[raw8BitPixels[pixelId]];
  }

  bool hasData() { return totalPixels > 0; }

  void clean() {
    if (!hasData())
      return;

    free(raw8BitPixels);
    free(raw15bppPalette);
  }
} Frame;

#endif  // FRAME_H
