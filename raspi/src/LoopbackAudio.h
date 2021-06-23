#ifndef LOOPBACK_AUDIO_H
#define LOOPBACK_AUDIO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <string>

#define AUDIO_COMMAND                                                  \
  "ffmpeg -f alsa -i hw:0,1,0 -y -ac 1 -af 'aresample=18157' -strict " \
  "unofficial -c:a gsm -f gsm -loglevel quiet -"
#define AUDIO_GSM_FRAME_SIZE 33

class LoopbackAudio {
 public:
  LoopbackAudio() { launchEncoder(); }

  uint8_t* loadChunk() {
    uint8_t* chunk = (uint8_t*)malloc(AUDIO_GSM_FRAME_SIZE);

    try {
      fseek(pipe, -AUDIO_GSM_FRAME_SIZE, SEEK_END);
      fread(chunk, AUDIO_GSM_FRAME_SIZE, 1, pipe);
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
