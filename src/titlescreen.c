#include "game.h"

static inline bool demoengine(struct gamestate *game) {
    return false;
}
static inline bool demoengine_step(struct gamestate *game) {
    return false;
}

static inline void titlescreen_startgame(struct gamestate *game, bool continuegame) {

}

static inline void titlescreen_menu(struct gamestate *game) {
    // run demo logic unless a button is pressed
    if (game->joypad1 == 0) {
        // check if it's time to run the demo
        if (game->timers.list.demo == 0) {
            // if so, check the demo state
            if (demoengine(game)) {
                // and keep running the demo while it's active
                demoengine_step(game);
                return;
            } else {
                // demo complete, return
                game->opermode = game->opermode_task = 0;
                return;
            }
        }
    } else {
        game->timers.list.demo = 0x1B;
    }

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
            game->timers.list.demo = 0x18;
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
