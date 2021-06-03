#include "SPISlave.h"
#include "Utils.h"

u32 SPISlave::transfer(u32 value) {
  setNormalMode();
  set32BitPackets();
  setSlaveMode();
  disableTransfer();

  setData(value);
  enableTransfer();
  startTransfer();

  while (!isReady())
    ;

  disableTransfer();
  u32 data = getData();

  return data;
}
