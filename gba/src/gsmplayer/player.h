#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

void player_init();
void player_play(const char* name);
void player_loop(const char* name);
void player_seek(unsigned int msecs);
void player_stop();
bool player_isPlaying();
void player_run();

#endif  // PLAYER_H
