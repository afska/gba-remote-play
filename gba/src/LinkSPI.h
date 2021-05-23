#ifndef LINK_SPI_H
#define LINK_SPI_H

#include <tonc.h>

#define SPI_BIT_CLOCK 0
#define SPI_BIT_SPEED 1
#define SPI_BIT_SI 2
#define SPI_BIT_SO 3
#define SPI_BIT_START 7
#define SPI_BIT_LENGTH 12
#define SPI_SET_HIGH(REG, BIT) REG |= 1 << BIT
#define SPI_SET_LOW(REG, BIT) REG &= ~(1 << BIT)
#define SPI_IS_HIGH(REG, BIT) ((REG >> BIT) & 1)

// A Link Port connection for Normal mode (slave, 2Mhz speed, 32bit packets)

class LinkSPI {
 public:
  u32 transfer(u32 value) {
    setNormalMode();
    set2MhzSpeed();
    set32BitPackets();
    setSlaveMode();
    disableTransfer();

    setData(value);
    enableTransfer();
    startTransfer();

    while (!isReady())
      ;

    u32 data = getData();
    disableTransfer();

    return data;
  }

 private:
  bool isWaiting = false;

  void setNormalMode() {
    REG_RCNT = 0;
    REG_SIOCNT = 0;
  }

  void setData(u32 data) { REG_SIODATA32 = data; }
  u32 getData() { return REG_SIODATA32; }

  void enableTransfer() { SPI_SET_LOW(REG_SIOCNT, SPI_BIT_SO); }
  void disableTransfer() { SPI_SET_HIGH(REG_SIOCNT, SPI_BIT_SO); }
  void startTransfer() { SPI_SET_HIGH(REG_SIOCNT, SPI_BIT_START); }
  bool isReady() { return !SPI_IS_HIGH(REG_SIOCNT, SPI_BIT_START); }

  void set2MhzSpeed() { SPI_SET_HIGH(REG_SIOCNT, SPI_BIT_SPEED); }
  void set32BitPackets() { SPI_SET_HIGH(REG_SIOCNT, SPI_BIT_LENGTH); }
  void setSlaveMode() { SPI_SET_LOW(REG_SIOCNT, SPI_BIT_CLOCK); }
};

extern LinkSPI* linkSPI;

#endif  // LINK_SPI_H
