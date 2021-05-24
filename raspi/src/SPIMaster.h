#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <iostream>

#ifdef __INTELLISENSE__
#define bswap_32(A) A
#define u_char unsigned char
#define wiringPiSPISetupMode(A, B, C)
#define wiringPiSPIDataRW(A, B, C)
#endif

#ifndef __INTELLISENSE__
#include <byteswap.h>
#include <wiringPiSPI.h>
#endif

#define CHANNEL 0
#define FREQUENCY 4440000
#define MODE 3

class SPIMaster {
 public:
  SPIMaster() { wiringPiSPISetupMode(CHANNEL, FREQUENCY, MODE); }

  uint32_t transfer(uint32_t value) {
    union {
      uint32_t u32;
      u_char uc[4];
    } x;

    x.u32 = bswap_32(value);
    wiringPiSPIDataRW(0, x.uc, 4);

    return bswap_32(x.u32);
  }
};

#endif  // SPI_MASTER_H
