#include <immintrin.h>
#include "game.h"

#ifdef __AVX2__
__m256i timers_all = { 0x0101010101010101,0x0101010101010101,0x0101010101010101, 0x0101010101010101 };
__m256i timers_frame = { 0x0101010101010101, 0x0101010101010101, 0, 0 };
#endif

static inline void timers_step(struct gamestate *game) {
    if (game->timercontrol > 0) {
        game->timercontrol -= 1;
        return;
    }
#ifndef __AVX2__
    int rulechange = game->timers.list.itc == 0;
    int mi = rulechange ? 0x20 : 0x10;
    for (int i=0; i<mi; ++i) {
        if (game->timers.array[i] > 0) {
            game->timers.array[i] -= 1;
        }
    }
    if (rulechange) {
        game->timers.list.itc = 0x14;
    }
#else
    if (game->timers.list.itc == 0) {
        game->timers.value = _mm256_subs_epu8(game->timers.value, timers_all);
        game->timers.list.itc = 0x14;
    } else {
        game->timers.value = _mm256_subs_epu8(game->timers.value, timers_frame);
    }
#endif
}
