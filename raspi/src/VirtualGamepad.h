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
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <vector>

#include "Utils.h"

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
#define VG_SEPARATOR_CONFIGURATION "<->"
#define VG_SEPARATOR_MAPPING "="
#define VG_SEPARATOR_COMBO "+"
#define VG_TOTAL_KEYS 10
#define VG_TOTAL_BUTTONS 14

// (key = gba, button = rpi)
const std::string KEY_NAMES[VG_TOTAL_KEYS] = {
    "LEFT", "RIGHT", "DOWN", "UP", "B", "A", "L", "R", "SELECT", "START"};
const std::string BUTTON_NAMES[VG_TOTAL_BUTTONS] = {
    "DPAD_LEFT", "DPAD_RIGHT", "DPAD_DOWN", "DPAD_UP", "SOUTH",
    "EAST",      "WEST",       "NORTH",     "TL",      "TR",
    "TL2",       "TR2",        "SELECT",    "START"};
const uint16_t KEYS[VG_TOTAL_KEYS] = {
    VG_KEY_LEFT, VG_KEY_RIGHT, VG_KEY_DOWN, VG_KEY_UP,     VG_KEY_B,
    VG_KEY_A,    VG_KEY_L,     VG_KEY_R,    VG_KEY_SELECT, VG_KEY_START};
const uint16_t BUTTONS[VG_TOTAL_BUTTONS] = {
    BTN_DPAD_LEFT, BTN_DPAD_RIGHT, BTN_DPAD_DOWN, BTN_DPAD_UP, BTN_SOUTH,
    BTN_EAST,      BTN_WEST,       BTN_NORTH,     BTN_TL,      BTN_TR,
    BTN_TL2,       BTN_TR2,        BTN_SELECT,    BTN_START};

typedef struct {
  std::vector<uint16_t> keys;
  uint16_t button;

  bool isValid() { return !keys.empty() && button != 0; }
  bool matches(uint16_t pressedKeys) {
    for (auto& key : keys)
      if (!(pressedKeys & key))
        return false;

    return true;
  }
} KeyMapping;

typedef struct {
  std::vector<KeyMapping> mappings;
} KeyConfiguration;

class VirtualGamepad {
 public:
  VirtualGamepad(std::string name, std::string fileName) {
    openUInput();
    configureKeys();
    registerDevice(name);

    std::ifstream file(fileName);
    std::string data((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    parseConfigurations(data);

    if (configurations.empty()) {
      std::cout << "Error (Input): invalid configuration, check " + fileName +
                       "\n";
      exit(44);
    }
  }

  void setCurrentConfiguration(uint32_t configurationId) {
    currentConfiguration =
        configurationId < configurations.size() ? configurationId : 0;
  }

  void setButtons(uint16_t pressedKeys) {
    auto configuration = configurations[currentConfiguration];

    for (int i = 0; i < VG_TOTAL_BUTTONS; i++) {
      auto button = BUTTONS[i];
      setButton(button, isPressed(button, pressedKeys, configuration));
    }

    flush();
  }

  ~VirtualGamepad() {
    ioctl(fileDescriptor, UI_DEV_DESTROY);
    close(fileDescriptor);
  }

 private:
  int fileDescriptor;
  std::vector<KeyConfiguration> configurations;
  uint32_t currentConfiguration = 0;

  bool isPressed(uint16_t button,
                 uint16_t pressedKeys,
                 KeyConfiguration& configuration) {
    for (auto& mapping : configuration.mappings)
      if (mapping.button == button && mapping.matches(pressedKeys))
        return true;

    return false;
  }

  void setButton(uint16_t button, bool isPressed) {
    struct input_event event;
    memset(&event, 0, sizeof(struct input_event));
    event.type = EV_KEY;
    event.code = button;
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
      std::cout << "Error (Input): cannot open uinput\n";
      exit(41);
    }
  }

  void configureKeys() {
    ioctl(fileDescriptor, UI_SET_EVBIT, EV_KEY);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_RIGHT);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_DPAD_UP);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_SOUTH);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_EAST);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_WEST);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_NORTH);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_TL);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_TR);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_TL2);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_TR2);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(fileDescriptor, UI_SET_KEYBIT, BTN_START);
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
      std::cout << "Error (Input): cannot write virtual gamepad settings\n";
      exit(42);
    }

    if (ioctl(fileDescriptor, UI_DEV_CREATE) < 0) {
      std::cout << "Error (Input): cannot create virtual gamepad device\n";
      exit(43);
    }
  }

  void parseConfigurations(std::string data) {
    auto rawConfigurations = split(data, VG_SEPARATOR_CONFIGURATION);

    for (auto& rawConfiguration : rawConfigurations) {
      auto configuration = parseConfiguration(rawConfiguration);
      configurations.push_back(configuration);
    }
  }

  KeyConfiguration parseConfiguration(std::string rawConfiguration) {
    KeyConfiguration configuration;
    auto lines = split(rawConfiguration, "\n");

    for (auto& line : lines) {
      auto mapping = parseLine(line);
      if (mapping.isValid())
        configuration.mappings.push_back(mapping);
    }

    return configuration;
  }

  KeyMapping parseLine(std::string line) {
    KeyMapping mapping;
    mapping.button = 0;

    auto parts = split(line, VG_SEPARATOR_MAPPING);
    if (parts.size() != 2)
      return mapping;

    auto rawKeys = parts.at(0);
    auto rawButton = parts.at(1);

    auto keyNames = split(rawKeys, VG_SEPARATOR_COMBO);
    for (auto& keyName : keyNames) {
      auto keyId = getKeyIdFromKeyName(keyName);
      if (keyId > -1)
        mapping.keys.push_back(KEYS[keyId]);
    }

    auto buttonId = getButtonIdFromButtonName(rawButton);
    if (buttonId > -1)
      mapping.button = BUTTONS[buttonId];

    return mapping;
  }

  int getKeyIdFromKeyName(std::string keyName) {
    for (int i = 0; i < VG_TOTAL_KEYS; i++) {
      if (KEY_NAMES[i] == keyName)
        return i;
    }

    return -1;
  }

  int getButtonIdFromButtonName(std::string buttonName) {
    for (int i = 0; i < VG_TOTAL_BUTTONS; i++) {
      if (BUTTON_NAMES[i] == buttonName)
        return i;
    }

    return -1;
  }
};

#endif  // VIRTUAL_GAMEPAD_H
