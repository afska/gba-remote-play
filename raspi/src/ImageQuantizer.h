#ifndef IMAGE_QUANTIZER_H
#define IMAGE_QUANTIZER_H

#include <stdint.h>
#include <stdlib.h>
#include "libimagequant.h"
#include "lodepng.h"  // TODO: REMOVE

#define QUANTIZER_MIN_SPEED 1
#define QUANTIZER_MAX_SPEED 8

typedef struct {
  uint8_t* raw8BitPixels;
  const liq_palette* palette;
} Frame;

class ImageQuantizer {
 public:
  uint8_t* quantize(uint8_t* rgbaPixels,
                    uint32_t width,
                    uint32_t height,
                    uint8_t speed) {
    liq_attr* handle = liq_attr_create();
    liq_image* inputImage =
        liq_image_create_rgba(handle, rgbaPixels, width, height, 0);

    liq_set_speed(handle, speed);

    liq_result* result;
    if (liq_image_quantize(inputImage, handle, &result) != LIQ_OK)
      return NULL;

    uint8_t* raw8BitPixels = remapImage(inputImage, result, width, height);
    const liq_palette* palette = liq_get_palette(result);

    // TODO: Serialize palette
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;

    for (int i = 0; i < palette->count; i++) {
      lodepng_palette_add(&state.info_png.color, palette->entries[i].r,
                          palette->entries[i].g, palette->entries[i].b,
                          palette->entries[i].a);
      lodepng_palette_add(&state.info_raw, palette->entries[i].r,
                          palette->entries[i].g, palette->entries[i].b,
                          palette->entries[i].a);
    }

    unsigned char* output_file_data;
    size_t output_file_size;
    unsigned int out_status =
        lodepng_encode(&output_file_data, &output_file_size, raw8BitPixels,
                       width, height, &state);
    if (out_status) {
      fprintf(stderr, "Can't encode image: %s\n",
              lodepng_error_text(out_status));
      return NULL;
    }

    const char* output_png_file_path = "quantized_example.png";
    FILE* fp = fopen(output_png_file_path, "wb");
    if (!fp) {
      fprintf(stderr, "Unable to write to %s\n", output_png_file_path);
      return NULL;
    }
    fwrite(output_file_data, 1, output_file_size, fp);
    fclose(fp);

    printf("Written %s\n", output_png_file_path);
    // ---

    liq_result_destroy(result);
    liq_image_destroy(inputImage);
    liq_attr_destroy(handle);

    return NULL;
  }

 private:
  uint8_t* remapImage(liq_image* image,
                      liq_result* result,
                      uint32_t width,
                      uint32_t height) {
    size_t size = width * height;
    uint8_t* raw8bitPixels = (uint8_t*)malloc(size);
    liq_set_dithering_level(result, 1.0);
    liq_write_remapped_image(result, image, raw8bitPixels, size);

    return raw8bitPixels;
  }
};

#endif  // IMAGE_QUANTIZER_H
