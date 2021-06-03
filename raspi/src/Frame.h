#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>

typedef struct Frame {
  uint32_t totalPixels;
  uint8_t* raw8BitPixels;
  uint16_t* raw15bppPalette;

  uint16_t getColorOf(uint32_t pixelId) {
    return raw15bppPalette[raw8BitPixels[pixelId]];
  }

  bool hasPixelChanged(uint32_t pixelId, Frame previousFrame) {
    return !previousFrame.hasData() ||
           areDifferent(getColorOf(pixelId), previousFrame.getColorOf(pixelId));
  }

  bool hasData() { return totalPixels > 0; }

  void clean() {
    if (!hasData())
      return;

    totalPixels = 0;
    free(raw8BitPixels);
    free(raw15bppPalette);
  }

 private:
  bool areDifferent(uint16_t color1, uint16_t color2) {
    return color1 != color2;
  }
} Frame;

#endif  // FRAME_H
