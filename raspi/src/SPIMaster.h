#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <iostream>

#include <byteswap.h>
#include <wiringPiSPI.h>

#define SPI_CHANNEL 0
#define SPI_FREQUENCY 4440000
#define SPI_MODE 3

class SPIMaster {
 public:
  SPIMaster() { wiringPiSPISetupMode(SPI_CHANNEL, SPI_FREQUENCY, SPI_MODE); }

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
