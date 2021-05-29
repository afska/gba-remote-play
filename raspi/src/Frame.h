#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t totalPixels = 0;
  uint8_t* raw8BitPixels = NULL;
  uint16_t* raw15bppPalette = NULL;

  bool isValid() { return raw8BitPixels != NULL && raw15bppPalette != NULL; }

  void clean() {
    if (raw8BitPixels)
      free(raw8BitPixels);

    if (raw15bppPalette)
      free(raw15bppPalette);
  }
} Frame;

#endif  // FRAME_H
