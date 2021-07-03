#ifndef PNG_WRITER_H
#define PNG_WRITER_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include "../lib/code/lodepng.h"

#define PNG_TARGET_COLORS 256

void WritePNG(std::string fileName,
              uint8_t* raw8BitPixels,
              const uint32_t* raw24bppPalette,
              uint32_t width,
              uint32_t height) {
#ifdef DEBUG_PNG
  LodePNGState state;
  lodepng_state_init(&state);
  state.info_raw.colortype = LCT_PALETTE;
  state.info_raw.bitdepth = 8;
  state.info_png.color.colortype = LCT_PALETTE;
  state.info_png.color.bitdepth = 8;

  for (int i = 0; i < PNG_TARGET_COLORS; i++) {
    uint32_t color = raw24bppPalette[i];
    uint8_t r = ((color >> 0) & 0xff);
    uint8_t g = ((color >> 8) & 0xff);
    uint8_t b = ((color >> 16) & 0xff);
    uint8_t a = 0xff;
    lodepng_palette_add(&state.info_png.color, r, g, b, a);
    lodepng_palette_add(&state.info_raw, r, g, b, a);
  }

  unsigned char* outputFileData;
  size_t outputFileSize;
  unsigned int outStatus = lodepng_encode(&outputFileData, &outputFileSize,
                                          raw8BitPixels, width, height, &state);
  if (outStatus) {
    fprintf(stderr, "Error (PNG): cannot encode image %s\n",
            lodepng_error_text(outStatus));
    exit(91);
  }

  const char* outputPath = fileName.c_str();
  FILE* file = fopen(outputPath, "wb");
  if (!file) {
    fprintf(stderr, "Error (PNG): unable to write to %s\n", outputPath);
    exit(92);
  }
  fwrite(outputFileData, 1, outputFileSize, file);
  fclose(file);

  printf("Written %s!\n", outputPath);
  lodepng_state_cleanup(&state);
#endif
}

#endif  // PNG_WRITER_H
