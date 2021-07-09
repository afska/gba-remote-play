#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

void player_init();
void player_play(const unsigned char* chunk, unsigned int size);
void player_stop();
void player_updateMediaSize(unsigned int newAddedBytes);
void player_onVBlank();
void player_run();

#endif  // PLAYER_H
