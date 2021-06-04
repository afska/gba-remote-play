#ifndef PNG_WRITER_H
#define PNG_WRITER_H

#include <stdint.h>
#include <stdio.h>
#include "../lib/code/lodepng.h"

#define PNG_SOURCE_COLORS 32
#define PNG_TARGET_COLORS 256

void WritePNG(std::string fileName,
              uint8_t* raw8BitPixels,
              uint16_t* raw15bppPalette,
              uint32_t width,
              uint32_t height) {
  LodePNGState state;
  lodepng_state_init(&state);
  state.info_raw.colortype = LCT_PALETTE;
  state.info_raw.bitdepth = 8;
  state.info_png.color.colortype = LCT_PALETTE;
  state.info_png.color.bitdepth = 8;

  for (int i = 0; i < PNG_TARGET_COLORS; i++) {
    uint16_t color = raw15bppPalette[i];
    uint8_t r = ((color >> 0) & 0xff) * PNG_TARGET_COLORS / PNG_SOURCE_COLORS;
    uint8_t g = ((color >> 5) & 0xff) * PNG_TARGET_COLORS / PNG_SOURCE_COLORS;
    uint8_t b = ((color >> 10) & 0xff) * PNG_TARGET_COLORS / PNG_SOURCE_COLORS;
    uint8_t a = 0xff;
    lodepng_palette_add(&state.info_png.color, r, g, b, a);
    lodepng_palette_add(&state.info_raw, r, g, b, a);
  }

  unsigned char* outputFileData;
  size_t outputFileSize;
  unsigned int out_status = lodepng_encode(
      &outputFileData, &outputFileSize, raw8BitPixels, width, height, &state);
  if (out_status) {
    fprintf(stderr, "Can't encode image: %s\n", lodepng_error_text(out_status));
    exit(91);
  }

  const char* outputPath = fileName.c_str();
  FILE* fp = fopen(outputPath, "wb");
  if (!fp) {
    fprintf(stderr, "Unable to write to %s\n", outputPath);
    exit(92);
  }
  fwrite(outputFileData, 1, outputFileSize, fp);
  fclose(fp);

  printf("Written %s!\n", outputPath);
  lodepng_state_cleanup(&state);
}

#endif  // PNG_WRITER_H
