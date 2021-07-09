#ifndef LOOPBACK_AUDIO_H
#define LOOPBACK_AUDIO_H

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "Protocol.h"

#define AUDIO_COMMAND_DRIVER "sudo modprobe snd-aloop"
#define AUDIO_COMMAND_ENCODER                                        \
  "ffmpeg -f alsa -i hw:0,1 -y -ac 1 -af 'aresample=18157' -strict " \
  "unofficial -c:a gsm -f gsm -loglevel quiet -"

#define AUDIO_NULL "/dev/null"

class LoopbackAudio {
 public:
  LoopbackAudio() {
#ifdef WITH_AUDIO
    launchEncoder();
#endif
  }

  uint8_t* loadChunk() {
#ifndef WITH_AUDIO
    return NULL;
#endif

    uint8_t* chunk = (uint8_t*)malloc(AUDIO_PADDED_SIZE);

    uint32_t availableBytes = consumeExtraChunks();

    if (availableBytes < AUDIO_CHUNK_SIZE ||
        read(pipeFd, chunk, AUDIO_CHUNK_SIZE) < 0) {
      free(chunk);
      return NULL;
    }

    return chunk;
  }

#ifdef WITH_AUDIO
  ~LoopbackAudio() {
    pclose(pipe);
    close(nullFd);
  }
#endif

 private:
  FILE* pipe;
  int nullFd;
  int pipeFd;

  void launchEncoder() {
    if (system(AUDIO_COMMAND_DRIVER) != 0) {
      std::cout << "Error (Audio): cannot start loopback audio driver\n";
      exit(31);
    }

    pipe = popen(AUDIO_COMMAND_ENCODER, "r");
    pipeFd = fileno(pipe);

    if (!pipe) {
      std::cout << "Error (Audio): cannot launch ffmpeg\n";
      exit(32);
    }

    if (fcntl(pipeFd, F_SETFL, O_NONBLOCK) < 0) {
      std::cout << "Error (Audio): cannot set non-blocking I/O\n";
      exit(33);
    }

    nullFd = open(AUDIO_NULL, O_RDWR);
  }

  uint32_t consumeExtraChunks() {
    uint32_t availableBytes = 0;

    ioctl(pipeFd, FIONREAD, &availableBytes);
    if (availableBytes > AUDIO_CHUNK_SIZE) {
      splice(pipeFd, NULL, nullFd, NULL, availableBytes - AUDIO_CHUNK_SIZE, 0);
      availableBytes = AUDIO_CHUNK_SIZE;
    }

    return availableBytes;
  }
};

#endif  // LOOPBACK_AUDIO_H
