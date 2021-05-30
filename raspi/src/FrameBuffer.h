#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <bcm_host.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include "Config.h"

#define FB_DEVFILE "/dev/fb0"
#define FB_BYTES_PER_PIXEL 4
#define FB_IMAGE_MODE VC_IMAGE_RGBA32

class FrameBuffer {
 public:
  FrameBuffer() {
    openFrameBuffer();
    retrieveFixedScreenInformation();
    retrieveVariableScreenInformation();
    allocateBuffer();

    if (DEBUG) {
      std::cout << "Resolution: " + std::to_string(variableInfo.xres) + "x" +
                       std::to_string(variableInfo.yres) + "\n";
      std::cout << "Buffer size: " + std::to_string(fixedInfo.smem_len) + "\n";
      std::cout << "Line length: " + std::to_string(fixedInfo.line_length) +
                       "\n";
    }

    openPrimaryDisplay();
    createScreenResource();
    setUpRect();
  }

  uint8_t* loadFrame() {
    vc_dispmanx_snapshot(display, screenResource, (DISPMANX_TRANSFORM_T)0);
    vc_dispmanx_resource_read_data(screenResource, &rect, buffer,
                                   variableInfo.xres * FB_BYTES_PER_PIXEL);

    return buffer;
  }

  ~FrameBuffer() {
    close(fileDescriptor);
    vc_dispmanx_resource_delete(screenResource);
    vc_dispmanx_display_close(display);
  }

 private:
  int fileDescriptor;
  struct fb_fix_screeninfo fixedInfo;
  struct fb_var_screeninfo variableInfo;
  uint8_t* buffer;
  DISPMANX_DISPLAY_HANDLE_T display;
  DISPMANX_RESOURCE_HANDLE_T screenResource;
  VC_IMAGE_TRANSFORM_T transform;
  uint32_t image_prt;
  VC_RECT_T rect;

  void openFrameBuffer() {
    fileDescriptor = open(FB_DEVFILE, O_RDWR);
    if (fileDescriptor == -1) {
      std::cout << "Error: cannot open framebuffer device\n";
      exit(1);
    }
  }

  void retrieveFixedScreenInformation() {
    if (ioctl(fileDescriptor, FBIOGET_FSCREENINFO, &fixedInfo) == -1) {
      std::cout << "Error: cannot read fixed information\n";
      exit(2);
    }
  }

  void retrieveVariableScreenInformation() {
    if (ioctl(fileDescriptor, FBIOGET_VSCREENINFO, &variableInfo) == -1) {
      std::cout << "Error: cannot read variable information\n";
      exit(3);
    }

    if (variableInfo.bits_per_pixel / 8 != FB_BYTES_PER_PIXEL) {
      std::cout << "Error: only 32bpp is supported\n";
      exit(4);
    }

    if (variableInfo.xres % FB_BYTES_PER_PIXEL != 0 ||
        variableInfo.yres % FB_BYTES_PER_PIXEL != 0) {
      std::cout << "Error: resolution must be word-aligned\n";
      exit(5);
    }
  }

  void allocateBuffer() {
    buffer = (uint8_t*)malloc(fixedInfo.smem_len);
    if (buffer == NULL) {
      std::cout << "Error: malloc(" + std::to_string(fixedInfo.smem_len) +
                       ") failed\n";
      exit(6);
    }
  }

  void openPrimaryDisplay() {
    bcm_host_init();

    display = vc_dispmanx_display_open(0);
    if (display == DISPMANX_NO_HANDLE) {
      std::cout << "Error: cannot open primary display\n";
      exit(7);
    }
  }

  void createScreenResource() {
    screenResource = vc_dispmanx_resource_create(
        FB_IMAGE_MODE, variableInfo.xres, variableInfo.yres, &image_prt);
    if (screenResource == DISPMANX_NO_HANDLE) {
      printf("Error: cannot create screen resource\n");
      exit(8);
    }
  }

  void setUpRect() {
    vc_dispmanx_rect_set(&rect, 0, 0, variableInfo.xres, variableInfo.yres);
  }
};

#endif  // FRAME_BUFFER_H
