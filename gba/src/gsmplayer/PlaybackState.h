#ifndef PLAYBACK_STATE_H
#define PLAYBACK_STATE_H

typedef struct {
  bool hasFinished;
  bool isLooping;
} Playback;

extern Playback PlaybackState;

#endif  // PLAYBACK_STATE_H
