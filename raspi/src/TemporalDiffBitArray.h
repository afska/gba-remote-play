#ifndef TEMPORAL_DIFF_BIT_ARRAY_H
#define TEMPORAL_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t data[TEMPORAL_DIFF_SIZE];

  void initialize(Frame currentFrame, Frame previousFrame) {
    for (int i = 0; i < TOTAL_PIXELS; i++)
      setBit(i, !previousFrame.hasData() ||
                    areDifferent(currentFrame.getColorOf(i),
                                 previousFrame.getColorOf(i)));
  }

 private:
  bool areDifferent(uint16_t color1, uint16_t color2) {
    return color1 != color2;
  }

  void setBit(uint32_t n, bool value) {
    uint8_t byte = n / 8;
    uint8_t bit = n % 8;

    data[byte] = value ? data[byte] | (1 << bit) : data[byte] & ~(1 << bit);
  }
} TemporalDiffBitArray;

#endif  // TEMPORAL_DIFF_BIT_ARRAY_H
