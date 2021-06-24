#ifndef LOOPBACK_AUDIO_H
#define LOOPBACK_AUDIO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "Protocol.h"

#define AUDIO_COMMAND                                                  \
  "ffmpeg -f alsa -i hw:0,1,0 -y -ac 1 -af 'aresample=18157' -strict " \
  "unofficial -c:a gsm -f gsm -loglevel quiet -"

class LoopbackAudio {
 public:
  LoopbackAudio() { launchEncoder(); }

  uint8_t* loadChunk() {
    uint8_t* chunk = (uint8_t*)malloc(AUDIO_PADDED_SIZE);
    try {
      fseek(pipe, 0, SEEK_END);
      while (!fread(chunk, AUDIO_CHUNK_SIZE, 1, pipe))
        ;
    } catch (...) {
      std::cout << "Audio error!\n";
    }

    return chunk;
  }

  ~LoopbackAudio() { pclose(pipe); }

 private:
  FILE* pipe;

  void launchEncoder() {
    pipe = popen(AUDIO_COMMAND, "r");
    if (!pipe) {
      std::cout << "Error: cannot launch ffmpeg\n";
      exit(31);
    }
  }
};

#endif  // LOOPBACK_AUDIO_H
