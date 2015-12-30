#ifndef REVOLC_GAME_GAME_H
#define REVOLC_GAME_GAME_H

#include "build.h"

// Stuff above engine level, but not game-specific -- things you'd see in main.c, but need to be shared.

#define SAVEFILE_PATH "save.bin"

REVOLC_API void make_main_blob(const char *blob_path, const char *game);
REVOLC_API const char *blob_path(const char *game);

#endif
