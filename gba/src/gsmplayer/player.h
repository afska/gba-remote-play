#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

void player_init();
void player_play(const unsigned char* chunk, unsigned int size);
void player_stop();
bool player_needsData();
void player_onVBlank();
void player_run();

#endif  // PLAYER_H
