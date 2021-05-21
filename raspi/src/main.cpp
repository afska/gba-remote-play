#include <byteswap.h>
#include <wiringPiSPI.h>
#include <iostream>

#ifdef __INTELLISENSE__
#define bswap_32(A) A
#define u_char unsigned char
#endif

uint32_t Spi32(uint32_t val) {
  union {
    uint32_t u32;
    u_char uc[4];
  } x;

  x.u32 = bswap_32(val);
  wiringPiSPIDataRW(0, x.uc, 4);

  return bswap_32(x.u32);
}

int main() {
  wiringPiSPISetupMode(0, 2000000, 3);

  do {
    uint32_t val = Spi32(2280);
    std::cout << (std::to_string(val) + "\n");
    if (val != 0) {
      std::cout << "SUCCESS\n";
      return 0;
    }
  } while (true);

  return 0;
}
