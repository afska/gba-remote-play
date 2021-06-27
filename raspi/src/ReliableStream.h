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

  bool send(void* data, uint32_t totalPackets) {
    uint32_t index = 0;
    lastReceivedPacket = 0;

    while (index < totalPackets) {
      uint32_t packetToSend = ((uint32_t*)data)[index];
      if (!sendPacket(packetToSend, &index, totalPackets))
        return false;
    }

    return true;
  }

  bool sync(uint32_t command) {
    lastReceivedPacket = 0;
    uint32_t local = command + CMD_RPI_OFFSET;
    uint32_t remote = command + CMD_GBA_OFFSET;
    uint32_t confirmation;

    while (true) {
      bool isOnSync = (confirmation = spiMaster->exchange(local)) == remote;

      if (isOnSync)
        return true;
      else {
        if (confirmation == CMD_RESET) {
          logReset("Reset! (sync)", local, remote);
          return false;
        }

        lastReceivedPacket = confirmation;
      }
    }
  }

 private:
  SPIMaster* spiMaster;
  uint32_t lastReceivedPacket = 0;

  bool sendPacket(uint32_t packet, uint32_t* index, uint32_t totalPackets) {
    if (*index % TRANSFER_SYNC_PERIOD == 0 || *index == totalPackets - 1) {
      return reliablySend(packet, index, totalPackets);
    } else {
      spiMaster->send(packet);
      (*index)++;
      return true;
    }
  }

  bool reliablySend(uint32_t packet, uint32_t* index, uint32_t totalPackets) {
    uint32_t requestedIndex = spiMaster->exchange(packet);
    if (requestedIndex != CMD_RESET)
      lastReceivedPacket = requestedIndex;

    if (requestedIndex == CMD_RECOVERY + CMD_GBA_OFFSET) {
      // (recovery command)
      if (!sync(CMD_RECOVERY))
        return false;
      requestedIndex = spiMaster->exchange(0);
      if (requestedIndex >= totalPackets) {
        logReset("Reset! (recovery)", packet, *index);
        return false;
      }
      *index = requestedIndex;
      return true;
    } else if (requestedIndex == CMD_RESET) {
      // (reset command)
      logReset("Reset! (stream)", packet, *index);
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

  void logReset(std::string title, uint32_t sent, uint32_t expected) {
#ifdef PROFILE_VERBOSE
    LOG(title);
    LOG("  [sent, expected, actual]");
    std::cout << "  0x" << std::hex << sent << "\n";
    std::cout << "  0x" << std::hex << expected << "\n";
    std::cout << "  0x" << std::hex << lastReceivedPacket << "\n";
#endif
  }
};

#endif  // RELIABLE_STREAM_H
