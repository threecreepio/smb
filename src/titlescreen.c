#include "game.h"

static const uint8_t demoinputs[] = {
    0x01, 0x00,
    0x01, 0x00,

    0x01, 0x80, 0x02, 0x81, 0x41, 0x80, 0x01,
    0x42, 0xc2, 0x02, 0x80, 0x41, 0xc1, 0x41, 0xc1,
    0x01, 0xc1, 0x01, 0x02, 0x80, 0x00
};

static const uint8_t demotiming[] = {
    0x40, 0xFF,
    0x40, 0xFF,

    0x9b, 0x10, 0x18, 0x05, 0x2c, 0x20, 0x24,
    0x15, 0x5a, 0x10, 0x20, 0x28, 0x30, 0x20, 0x10,
    0x80, 0x20, 0x30, 0x30, 0x01, 0xff, 0x00
};

static inline int demoengine(struct gamestate *game) {
    // are we advancing to the next demo action?
    if (game->timers.list.demoaction == 0xFF) {
        // yep - get timing for next step
        game->timers.list.demoaction = demotiming[game->demostep];
        // if timing is 0 then we're finished running the demo, and should reset.
        if (game->timers.list.demoaction == 0) return 1;
        // otherwise advance to next demo step.
        game->demostep += 1;
    }
    // get inputs for the currently running demo step.
    game->joypad1 = demoinputs[game->demostep - 1];
    // and keep running the demo.
    return 0;
}

static inline int demoengine_step(struct gamestate *game) {
    return 0;
}

static inline void titlescreen_startgame(struct gamestate *game, int continuegame) {

}

static inline void titlescreen_menu(struct gamestate *game) {
    // should we run the demo?
    if (game->timers.list.itc_demo == 0) {
        // cancel demo if player presses buttons
        int end = game->joypad1 != 0;
        // otherwise, check if we have more demo to play
        end = end || demoengine(game);
        // if so, run a step of the demo
        end = end || demoengine_step(game);
        if (end) {
            // if the demo should end, reset some state.
            game->timers.list.itc_demo = 0x1B;
            game->opermode = game->opermode_task = 0;
            game->demostep = 0;
            return;
        }
    }
    gamecore(game);

    // check if we want to change player count
    if ((game->joypad1 & INPUT_SELECT) && game->timers.list.control == 0) {
        // if so - increment player count but keep it in range.
        game->number_of_players = (game->number_of_players + 1) % 2;
        game->timers.list.control = 0x10;
    }

    // check if we want to change starting world
    if (game->worldselectenabled && (game->joypad1 & INPUT_B) && game->timers.list.control == 0) {
        // if so - increment world number but keep it in range.
        game->worldnumber = (game->worldnumber + 1) % 7;
        game->timers.list.control = 0x10;
    }

    // start game if player pressed start
    if (game->joypad1 & INPUT_START) {
        titlescreen_startgame(game, (game->joypad1 & INPUT_A) > 0);
    }
}

static inline void titlescreen_step(struct gamestate *game) {
    switch (game->opermode_task) {
        case 0:
            game->timers.list.itc_demo = 0x1B;
            game->opermode_task += 1;
            break;
        case 1:
            titlescreen_menu(game);
            break;
        default:
            game->opermode_task = 0;
            break;
    }
}
