#ifndef LINUX_FRAME_BUFFER_H
#define LINUX_FRAME_BUFFER_H

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define FB_MIN_BUFFER_SIZE 128
#define FB_BITS_PER_PIXEL 32
#define FB_STEP 2

// Pixel specification 16bpp
typedef short pixel_t;

// Framebuffer pointer
typedef pixel_t* fbp_t;

class LinuxFrameBuffer {
 public:
  LinuxFrameBuffer() {
    openFrameBuffer();
    retrieveFixedScreenInformation();
    retrieveVariableScreenInformation();
    mapFrameBufferIntoMemory();
  }

  template <typename F>
  inline void forEachPixel(F action) {
    unsigned int numberOfPixels = screenSizeBytes / sizeof(pixel_t);

    for (int i = 0; i < numberOfPixels; i += 2) {
      if (variableInfo.width < FB_MIN_BUFFER_SIZE &&
          i % FB_MIN_BUFFER_SIZE >= variableInfo.width)
        continue;

      uint8_t red = buffer[i + 1] & 0xff;
      uint8_t green = (buffer[i] >> 8) & 0xff;
      uint8_t blue = buffer[i] & 0xff;

      action(red, green, blue);
    }
  }

  ~LinuxFrameBuffer() {
    munmap(buffer, screenSizeBytes);
    close(fileDescriptor);
  }

 private:
  int fileDescriptor;
  struct fb_var_screeninfo variableInfo;
  struct fb_fix_screeninfo fixedInfo;
  unsigned long int screenSizeBytes;  // (width * height * bpp / 8)
  fbp_t buffer;

  void openFrameBuffer() {
    fileDescriptor = open("/dev/fb0", O_RDWR);
    if (fileDescriptor == -1) {
      printf("Error: cannot open framebuffer device\n");
      exit(1);
    }
  }

  void retrieveFixedScreenInformation() {
    if (ioctl(fileDescriptor, FBIOGET_FSCREENINFO, &fixedInfo) == -1) {
      printf("Error: cannot read fixed information\n");
      exit(2);
    }
  }

  void retrieveVariableScreenInformation() {
    if (ioctl(fileDescriptor, FBIOGET_VSCREENINFO, &variableInfo) == -1) {
      printf("Error: cannot read variable information\n");
      exit(3);
    }

    screenSizeBytes =
        variableInfo.xres * variableInfo.yres * variableInfo.bits_per_pixel / 8;
  }

  void mapFrameBufferIntoMemory() {
    buffer = (fbp_t)mmap(0, screenSizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED,
                         fileDescriptor, 0);

    if ((int)buffer == -1) {
      printf("Error: failed to map framebuffer device to memory\n");
      exit(4);
    }
  }
};

#endif  // LINUX_FRAME_BUFFER_H
