#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <tonc.h>

#include "SPISlave.h"

class SPIMaster {
 public:
  u32 transfer(u32 value) {
    setNormalMode();
    set32BitPackets();
    set2MhzSpeed();
    setMasterMode();

    setData(value);
    enableTransfer();

    while (!isSlaveReady())
      ;

    // waste some cycles
    for (u32 i = 0; i < 20; i++)
      pal_obj_mem[i] = i;

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
  bool isSlaveReady() { return !BIT_IS_HIGH(REG_SIOCNT, SPI_BIT_SI); }
  bool isReady() { return !BIT_IS_HIGH(REG_SIOCNT, SPI_BIT_START); }

  void set32BitPackets() { BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_LENGTH); }
  void set2MhzSpeed() { BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_CLOCK_SPEED); }
  void setMasterMode() { BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_CLOCK); }
};

extern SPIMaster* spiMaster;

#endif  // SPI_MASTER_H
