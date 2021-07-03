#ifndef GPIO_H
#define GPIO_H

#include <tonc.h>

#define GPIO_MODE 15
#define GPIO_SI_INTERRUPT_BIT 8

#define REG_SET_BIT(REG, BIT, DATA) \
  if (DATA)                         \
    BIT_SET_HIGH(REG, BIT);         \
  else                              \
    BIT_SET_LOW(REG, BIT);

#define BIT_SET_HIGH(REG, BIT) REG |= 1 << BIT
#define BIT_SET_LOW(REG, BIT) REG &= ~(1 << BIT)
#define BIT_IS_HIGH(REG, BIT) ((REG >> BIT) & 1)

enum Pin { SI, SO, SD, SC };
enum PinDirection { INPUT, OUTPUT };
const u8 PIN_DATA_BITS[] = {2, 3, 1, 0};
const u8 PIN_DIRECTION_BITS[] = {6, 7, 5, 4};

inline void GPIO_enable() {
  REG_RCNT = 1 << GPIO_MODE;
  REG_SIOCNT = 0;
}

inline void GPIO_setMode(Pin pin, PinDirection direction) {
  REG_SET_BIT(REG_RCNT, PIN_DIRECTION_BITS[pin],
              direction == PinDirection::OUTPUT);
}

inline bool GPIO_getData(Pin pin) {
  return (REG_RCNT & (1 << PIN_DATA_BITS[pin])) != 0;
}

inline void GPIO_setData(Pin pin, bool data) {
  REG_SET_BIT(REG_RCNT, PIN_DATA_BITS[pin], data);
}

inline void GPIO_setSIInterrupts(bool isEnabled) {
  REG_SET_BIT(REG_RCNT, GPIO_SI_INTERRUPT_BIT, isEnabled);
}

#endif  // GPIO_H
