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

  bool send(void* data, uint32_t size) {
    uint32_t index = 0;

    while (index < size) {
      uint32_t packetToSend = ((uint32_t*)data)[index];
      if (!sendPacket(packetToSend, &index, size))
        return false;
    }

    return true;
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
        if (confirmation == CMD_RESET && lastReceivedPacket < CMD_BUSY) {
          LOG("Reset!");
          LOG("  [sent, expected, actual]");
          std::cout << "  0x" << std::hex << local << "\n";
          std::cout << "  0x" << std::hex << remote << "\n";
          std::cout << "  0x" << std::hex << lastReceivedPacket << "\n";
          return false;
        }

        lastReceivedPacket = confirmation;
      }
    }
  }

 private:
  SPIMaster* spiMaster;

  bool sendPacket(uint32_t packet, uint32_t* index, uint32_t size) {
    if (*index % TRANSFER_SYNC_PERIOD == 0 || *index == size - 1) {
      return reliablySend(packet, index);
    } else {
      spiMaster->send(packet);
      (*index)++;
      return true;
    }
  }

  bool reliablySend(uint32_t packet, uint32_t* index) {
    uint32_t requestedIndex = spiMaster->exchange(packet);

    if (requestedIndex == CMD_RECOVERY + CMD_GBA_OFFSET) {
      // (recovery command)
      if (!sync(CMD_RECOVERY))
        return false;
      requestedIndex = spiMaster->exchange(0);
      *index = requestedIndex;
      return true;
    } else if (requestedIndex == CMD_RESET) {
      // (reset command)
      return false;
    } else if (requestedIndex == *index) {
      // (on sync)
      (*index)++;
      return true;
    } else {
      // (probably garbage => ignore)
      return true;
    }
  }
};

#endif  // RELIABLE_STREAM_H
