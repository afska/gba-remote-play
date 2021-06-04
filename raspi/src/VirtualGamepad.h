#ifndef VIRTUAL_GAMEPAD_H
#define VIRTUAL_GAMEPAD_H

#include <linux/input.h>
#include <linux/uinput.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#define VG_DEVFILE "/dev/uinput"
#define VG_KEY_A 0x0001
#define VG_KEY_B 0x0002
#define VG_KEY_SELECT 0x0004
#define VG_KEY_START 0x0008
#define VG_KEY_RIGHT 0x0010
#define VG_KEY_LEFT 0x0020
#define VG_KEY_UP 0x0040
#define VG_KEY_DOWN 0x0080
#define VG_KEY_R 0x0100
#define VG_KEY_L 0x0200

class VirtualGamepad {
 public:
  VirtualGamepad(std::string name) {
    openUInput();
    configureKeys();
    registerDevice(name);
  }

  void setKeys(uint16_t keys) {
    setKey(BTN_EAST, keys & VG_KEY_A);
    setKey(BTN_SOUTH, keys & VG_KEY_B);
    setKey(BTN_SELECT, keys & VG_KEY_SELECT);
    setKey(BTN_START, keys & VG_KEY_START);
    setKey(BTN_DPAD_RIGHT, keys & VG_KEY_RIGHT);
    setKey(BTN_DPAD_LEFT, keys & VG_KEY_LEFT);
    setKey(BTN_DPAD_UP, keys & VG_KEY_UP);
    setKey(BTN_DPAD_DOWN, keys & VG_KEY_DOWN);
    setKey(BTN_TR, keys & VG_KEY_R);
    setKey(BTN_TL, keys & VG_KEY_L);

    flush();
  }

  ~VirtualGamepad() {
    ioctl(fileDescriptor, UI_DEV_DESTROY);
    close(fileDescriptor);
  }

 private:
  int fileDescriptor;

  void setKey(uint16_t key, bool isPressed) {
    struct input_event event;
    memset(&event, 0, sizeof(struct input_event));
    event.type = EV_KEY;
    event.code = key;
    event.value = isPressed;
    write(fileDescriptor, &event, sizeof(struct input_event));
  }

  void flush() {
    struct input_event event;
    memset(&event, 0, sizeof(struct input_event));
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    write(fileDescriptor, &event, sizeof(struct input_event));
  }

  void openUInput() {
    fileDescriptor = open(VG_DEVFILE, O_WRONLY | O_NONBLOCK);
    if (fileDescriptor < 0) {
      std::cout << "Error: cannot open framebuffer device\n";
      exit(31);
    }
  }

  void configureKeys() {
    ioctl(fileDescriptor, UI_SET_EVBIT, EV_KEY);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_A);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_B);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_TL);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_TR);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_START);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_UP);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
  }

  void registerDevice(std::string name) {
    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, name.c_str());
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x3;
    uidev.id.product = 0x3;
    uidev.id.version = 2;

    if (write(fileDescriptor, &uidev, sizeof(uidev)) < 0) {
      std::cout << "Error: cannot write virtual gamepad settings\n";
      exit(32);
    }

    if (ioctl(fileDescriptor, UI_DEV_CREATE) < 0) {
      std::cout << "Error: cannot create virtual gamepad device\n";
      exit(33);
    }
  }
};

#endif  // VIRTUAL_GAMEPAD_H
