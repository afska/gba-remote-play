#include "_state.h"
#include "RuntimeConfig.h"
#include "Utils.h"

DATA_IWRAM State state;
DATA_IWRAM Config config;
DATA_IWRAM SPISlave* spiSlave = new SPISlave();
DATA_EWRAM u8 compressedPixels[MAX_PIXELS_SIZE];
