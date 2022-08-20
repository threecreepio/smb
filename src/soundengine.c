#include "game.h"

#define SFX_PAUSE 0
#define SFX_UNPAUSE 1
#define SFX_ENEMYSTOMP 2
#define SFX_JUMP_BIG 3
#define SFX_JUMP_SMALL 4

static inline void sound_play(struct gamestate *game, int sound) {}
static void sound_step(struct gamestate *game) {}
