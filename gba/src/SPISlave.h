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

class SPISlave {
 public:
  SPISlave() { start(); }

  void start() {
    setNormalMode();
    set32BitPackets();
    setSlaveMode();
    disableTransfer();
  }

  u32 transfer(u32 value) {
    return transfer(
        value, []() { return false; }, NULL);
  }

  template <typename F>
  u32 transfer(u32 value, F needsBreak, bool* breakFlag) {
    setData(value);
    enableTransfer();
    startTransfer();

    while (!isReady()) {
      if (needsBreak()) {
        setData(0xffffffff);
        disableTransfer();
        *breakFlag = true;
        return 0;
      }
    }

    disableTransfer();
    u32 data = getData();

    return data;
  }

  void stop() {
    stopTransfer();
    disableTransfer();
    BIT_SET_LOW(REG_SIOCNT, SPI_BIT_IRQ);
    BIT_SET_HIGH(REG_SIOCNT, SPI_BIT_IRQ);
    // (
    //  This doesn't make any sense, but it somehow fixes a ~random~ CPU crash
    //  when using DMA1 for audio and SPI transfers. Source: experimentation.
    // )
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
