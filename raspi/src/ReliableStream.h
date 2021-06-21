#ifndef RELIABLE_STREAM_H
#define RELIABLE_STREAM_H

#include <stdint.h>
#include <iostream>
#include "Protocol.h"
#include "SPIMaster.h"
#include "Utils.h"

class ReliableStream {
 public:
  ReliableStream(SPIMaster* spiMaster) { this->spiMaster = spiMaster; }

  bool send(uint32_t packet, uint32_t* index) {
    if (*index % TRANSFER_SYNC_FREQUENCY == 0) {
      return reliablySend(packet, index);
    } else {
      spiMaster->send(packet);
      return true;
    }
  }

  bool sync(uint32_t command) {
    uint32_t local = command + CMD_RPI_OFFSET;
    uint32_t remote = command + CMD_GBA_OFFSET;
    uint32_t confirmation;
    uint32_t lastReceivedPacket = 0;

    while (true) {
      bool isOnSync = true;
      for (int i = 0; i < SYNC_VALIDATIONS; i++)
        isOnSync =
            isOnSync &&
            (confirmation = spiMaster->exchange(local + i)) == remote + i;

      if (isOnSync)
        return true;
      else {
        if (confirmation == CMD_RESET) {
          LOG("Reset! (sent, expected, actual)");
          std::cout << "0x" << std::hex << local << "\n";
          std::cout << "0x" << std::hex << remote << "\n";
          std::cout << "0x" << std::hex << lastReceivedPacket << "\n\n";
          return false;
        }

        lastReceivedPacket = confirmation;
      }
    }
  }

 private:
  SPIMaster* spiMaster;

 private:
  bool reliablySend(uint32_t packet, uint32_t* index) {
    uint32_t requestedIndex = spiMaster->exchange(packet);

    if (requestedIndex >= MIN_COMMAND) {
      if (requestedIndex == CMD_RECOVERY + CMD_GBA_OFFSET) {
        // (recovery command)
        sync(CMD_RECOVERY);
        requestedIndex = spiMaster->exchange(0);
        *index = requestedIndex;
        return true;
      } else {
        // (unknown command => reset)
        return false;
      }
    } else if (requestedIndex == *index) {
      // (on sync)
      (*index)++;
      return true;
    } else {
      // (not on sync => reset)
      return false;
    }
  }
};

#endif  // RELIABLE_STREAM_H
