#ifndef IMAGE_QUANTIZER_H
#define IMAGE_QUANTIZER_H

#include <stdint.h>
#include <stdlib.h>
#include "Frame.h"
#include "libimagequant.h"

#define QUANTIZER_MIN_SPEED 1
#define QUANTIZER_MAX_SPEED 10
#define QUANTIZER_8BIT_MAX_COLORS 256
#define QUANTIZER_5BIT_MAX_COLORS 32

class ImageQuantizer {
 public:
  Frame quantize(uint8_t* rgbaPixels,
                 uint32_t width,
                 uint32_t height,
                 uint8_t speed = QUANTIZER_MAX_SPEED) {
    liq_attr* handle = liq_attr_create();
    liq_image* inputImage =
        liq_image_create_rgba(handle, rgbaPixels, width, height, 0);

    liq_set_speed(handle, speed);

    liq_result* result;
    if (liq_image_quantize(inputImage, handle, &result) != LIQ_OK)
      return Frame{0};

    auto frame = Frame{};
    frame.totalPixels = width * height;
    frame.raw8BitPixels = remapImage(inputImage, result, frame.totalPixels);
    frame.raw15bppPalette = serializePalette(result, frame.totalPixels);

    liq_result_destroy(result);
    liq_image_destroy(inputImage);
    liq_attr_destroy(handle);

    return frame;
  }

 private:
  uint8_t* remapImage(liq_image* image,
                      liq_result* result,
                      uint32_t totalPixels) {
    size_t size = totalPixels;
    uint8_t* raw8bitPixels = (uint8_t*)malloc(size);

    liq_set_dithering_level(result, 0.0);
    liq_write_remapped_image(result, image, raw8bitPixels, size);

    return raw8bitPixels;
  }

  uint16_t* serializePalette(liq_result* result, uint32_t totalPixels) {
    size_t size = totalPixels * 2;
    uint16_t* raw15bppPalette = (uint16_t*)malloc(size);

    const liq_palette* palette = liq_get_palette(result);

    for (int i = 0; i < palette->count; i++) {
      uint8_t red = palette->entries[i].r;
      uint8_t green = palette->entries[i].g;
      uint8_t blue = palette->entries[i].b;

      uint8_t r = red * QUANTIZER_5BIT_MAX_COLORS / QUANTIZER_8BIT_MAX_COLORS;
      uint8_t g = green * QUANTIZER_5BIT_MAX_COLORS / QUANTIZER_8BIT_MAX_COLORS;
      uint8_t b = blue * QUANTIZER_5BIT_MAX_COLORS / QUANTIZER_8BIT_MAX_COLORS;

      raw15bppPalette[i] =
          palette->entries[i].a != 0 ? r | (g << 5) | (b << 10) : 0;
    }

    return raw15bppPalette;
  }
};

#endif  // IMAGE_QUANTIZER_H
