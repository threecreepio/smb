#include "game.h"

#define RNG(i) (rng.list[0x6-i])

static inline void rng_step(struct gamestate *game) {
    if (!game->rng.value) game->rng.value = 0x00A5000000000000;
    uint64_t carry = (game->rng.value & 0x02000000000000) ^ ((game->rng.value << 8) & 0x02000000000000) ? 0x0080000000000000 : 0;
    game->rng.value = (game->rng.value >> 1) | carry;
}
