#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <iostream>

#include <byteswap.h>
#include "bcm2835.h"

#define SPI_CHANNEL 0
#define SPI_MODE 3
#define SPI_FREQUENCY 1600000
#define SPI_DELAY_MICROSECONDS 5

class SPIMaster {
 public:
  SPIMaster() {
    initialize();
    bcm2835_spi_set_speed_hz(SPI_FREQUENCY);
    bcm2835_spi_setDataMode(SPI_MODE);
  }

  uint32_t transfer(uint32_t value) {
    union {
      uint32_t u32;
      char uc[4];
    } x;

    x.u32 = bswap_32(value);
    bcm2835_delayMicroseconds(SPI_DELAY_MICROSECONDS);
    bcm2835_spi_transfern(x.uc, 4);
    return bswap_32(x.u32);
  }

  ~SPIMaster() { bcm2835_spi_end(); }

 private:
  void initialize() {
    if (!bcm2835_init()) {
      std::cout << "Error: cannot initialize SPI\n";
      exit(11);
    }

    if (!bcm2835_spi_begin()) {
      std::cout << "Error: cannot start SPI transfers\n";
      exit(12);
    }
  }
};

#endif  // SPI_MASTER_H
