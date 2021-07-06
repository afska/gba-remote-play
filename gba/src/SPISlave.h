#ifndef SPI_SLAVE_H
#define SPI_SLAVE_H

#include <tonc.h>

#include "Utils.h"

#define SPI_BIT_CLOCK 0
#define SPI_BIT_SI 2
#define SPI_BIT_SO 3
#define SPI_BIT_START 7
#define SPI_BIT_LENGTH 12
#define SPI_BIT_IRQ 14

// A Link Port connection for Normal mode (slave, 32bit packets)

class SPISlave {
 public:
  SPISlave() {
    setNormalMode();
    set32BitPackets();
    setSlaveMode();
    disableTransfer();
  }

  u32 transfer(u32 value) {
    setData(value);
    enableTransfer();
    startTransfer();

    while (!isReady())
      ;

    disableTransfer();
    u32 data = getData();

    return data;
  }

 private:
  void setNormalMode() {
    REG_RCNT = 0;
    REG_SIOCNT = 0;
  }

  void setData(u32 data) { REG_SIODATA32 = data; }
  u32 getData() { return REG_SIODATA32; }

  void enableTransfer() { BIT_SET_LOW(REG_SIOCNT, SPI_BIT_SO); }
  void disableTransfer() { BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_SO); }
  void startTransfer() { BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_START); }
  void stopTransfer() { BIT_SET_LOW(REG_SIOCNT, SPI_BIT_START); }
  bool isReady() { return !BIT_IS_HIGH(REG_SIOCNT, SPI_BIT_START); }

  void set32BitPackets() { BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_LENGTH); }
  void setSlaveMode() { BIT_SET_LOW(REG_SIOCNT, SPI_BIT_CLOCK); }
};

extern SPISlave* spiSlave;

#endif  // SPI_SLAVE_H
