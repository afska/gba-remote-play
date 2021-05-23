#include <unistd.h>
#include <iostream>

#ifdef __INTELLISENSE__
#define bswap_32(A) A
#define gpioInitialise() 0
#define spiOpen(A, B, C) 0
#define spiXfer(A, B, C, D) 0
#endif
#ifndef __INTELLISENSE__
#include <byteswap.h>
extern "C" {
#include <pigpio.h>
}
#endif

int spiHandle = 0;

uint32_t Spi32(uint32_t val) {
  union {
    uint32_t u32;
    char uc[4];
  } output;

  union {
    uint32_t u32;
    char uc[4];
  } input;

  output.u32 = bswap_32(val);
  spiXfer(spiHandle, output.uc, input.uc, 4);

  return bswap_32(input.u32);
}

int main() {
  if (gpioInitialise() < 0)
    exit(1);
  if ((spiHandle = spiOpen(0, 2000000, 0)) < 0)
    exit(2);

  std::cout << "Starting...\n";

  while (true) {
    uint32_t val = Spi32(0x98765432);
  }

  return 0;
}
