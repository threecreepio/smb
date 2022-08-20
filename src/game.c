#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "game.h"
#include "titlescreen.c"
#include "timers.c"
#include "rng.c"
#include "soundengine.c"
#include "areas.c"

static inline uint8_t pause_step(struct gamestate *game) {
    // cant be paused outside game state
    if (game->opermode == OPERMODE_TITLE || game->opermode == OPERMODE_GAMEOVER) return 0;
    // if we're in game, but not running the game engine, we can't pause.
    if (game->opermode == OPERMODE_GAME && game->opermode_task != 0x3) {
        return 0;
    }
    // if we've recently paused, count down the timer.
    if (game->pause_timer != 0) {
        game->pause_timer -= 1;
        return game->pause_state & 0b1;
    }
    // exit out if joypad1 is not pressed
    if ((game->joypad1 & INPUT_START) == 0) {
        game->pause_state &= 0b01111111;
        return game->pause_state & 0b1;
    }
    // if we're still holding start, do not invert pause
    if (game->pause_state & 0b10000000) return 0;
    // otherwise set pause timer, play the sound effect, and flip pause state
    game->pause_timer = 0x2B;
    sound_play(game, game->pause_state & 1 ? SFX_UNPAUSE : SFX_PAUSE);
    game->pause_state = 0b10000000 | (game->pause_state ^ 0b1);
    return game->pause_state & 0b1;
}

static inline void nmi(struct gamestate *game) {
    sound_step(game);
    if (!pause_step(game)) {
        game->framecounter += 1;
        timers_step(game);
        rng_step(game);
        switch (game->opermode) {
            case OPERMODE_TITLE: titlescreen_step(game); break;
        }
    }
    game->joypad1_previous = game->joypad1;
}
