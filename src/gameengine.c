#include "game.c"

static inline void gameroutines_setup(struct gamestate *game) {
    game->entities.x[0] = game->scrollx & 0xFF00;
    game->gameengine_task = 7;
}

static inline void gameroutines_entrance(struct gamestate *game) {
}

static inline void gameroutines_playerphysics(struct gamestate *game) {
    if (game->player.state == PLAYERSTATE_CLIMBING) {
        // check player climb direction
        uint8_t dir = (INPUT_UP | INPUT_DOWN) | (game->joypad1 & game->entities.collision[0]);
        if (dir & INPUT_UP) game->entities.yspeed[0] = -0xe0;
        else if (dir & INPUT_DOWN) game->entities.yspeed[0] = 0x1ff;
        else game->entities.yspeed[0] = 0;
        game->player.animationspeed = 8 >> (dir & INPUT_UP ? 1 : 0);
        return;
    }

    // check if we're trying to jump
    if (game->jumpspring_state == 0 && (game->joypad1_previous & INPUT_A) == 0 && (game->joypad1 & INPUT_A) > 0) {
        // then check so we can actually jump
        if (game->player.state == 0 || (game->player.swimming && (game->timers.list.jumpswim == 0 || (game->entities.yspeed[0] > 8)))) {
            // change to jumping state
            game->player.state = PLAYERSTATE_JUMPSWIM;
            // set swimming cooldown timer.
            game->timers.list.jumpswim = 0x20;
            // clear subpixel y speed
            game->player.ymf_dummy = 0;
            game->entities.yspeed[0] &= 0xFF00;
            // and set our current position as the origin of the jump
            game->player.jumporigin_y = game->entities.y[0];
            if (game->player.swimming) {
                // play swimming sound
                sound_play(game, SFX_ENEMYSTOMP);
                // get swim movement based on whirlpool state
                if (game->player.inwhirlpool) {
                    game->player.verticalforceup = 0x04;
                    game->player.verticalforcedown = 0x09;
                    game->entities.yspeed[0] = 0xFF00;
                } else {
                    game->player.verticalforceup = 0x0D;
                    game->player.verticalforcedown = 0x0A;
                    game->entities.yspeed[0] = 0xFE80;
                }
                // if player is too close to the top of the screen, cancel upward movement
                if (game->entities.y[0] <= 0x1400) {
                    game->entities.yspeed[0] &= 0x00FF;
                }
            } else {
                // play normal jumping sound
                sound_play(game, game->player.size ? SFX_JUMP_SMALL : SFX_JUMP_BIG);
                // and get movement forces based on players movement speed
                int16_t speed = game->player.xspeed_absolute;
                if (speed >= 0x1900) {
                    game->player.verticalforceup = 0x28;
                    game->player.verticalforcedown = 0x90;
                    game->entities.yspeed[0] = 0xFB00;
                } else if (speed >= 0x1000) {
                    game->player.verticalforceup = 0x1E;
                    game->player.verticalforcedown = 0x60;
                    game->entities.yspeed[0] = 0xFC00;
                } else {
                    game->player.verticalforceup = 0x20;
                    game->player.verticalforcedown = 0x70;
                    game->entities.yspeed[0] = 0xFC00;
                }
            }
        }
    }

    // x physics
    bool rfast = false;
    if (game->player.state != PLAYERSTATE_ONGROUND) {
        rfast = game->player.xspeed_absolute <= 0x1900;
    } else if (game->player.swimming) {
        rfast = true;
    } else if ((game->joypad1 & game->player.movingdir) == 0) {
        rfast = true;
    } else {
        rfast = game->joypad1 & INPUT_B;
    }

    if (game->player.runningspeed == 0) {

    }

    // double friction value if moving against facing direction
    if (game->player.movingdir != game->player.facing) {
        game->player.friction <<= 1;
    }
}

static inline void gameroutines_playermovement(struct gamestate *game) {
    // if the player is large and holding the down button while on land, set crouch flag
    if (game->player.size == PLAYERSIZE_SMALL) {
        game->player.crouching = false;
    } else if (game->player.state == PLAYERSTATE_ONGROUND) {
        game->player.crouching = game->joypad1 & INPUT_DOWN;
    }

    gameroutines_playerphysics(game);

    // skip movement while size change animation is running
    if (game->player.changingsize) {
        return;
    }

    // if player is climbing, keep resetting climb timer
    if (game->player.state == PLAYERSTATE_CLIMBING) {
        game->timers.list.climbside = 0x18;
    }

    // run movement routine based on player state
    switch (game->player.state) {
        case PLAYERSTATE_ONGROUND:
            do {
                if ((game->entities.xspeed[0] & 0x7F00) == 0) {
                    // we are standing still, no need for friction.
                    game->player.xspeed_absolute = 0;
                    break;
                }
                // friction
                uint8_t lrinputs = game->joypad1 & (INPUT_LEFT | INPUT_RIGHT);
                if (lrinputs) game->player.facing = lrinputs;
                // check if we're colliding in the direction we are trying to move
                if ((lrinputs & game->entities.collision[0]) == 0) {

                }
            } while (0);

            break;
        case PLAYERSTATE_JUMPSWIM:
            break;
        case PLAYERSTATE_FALLING:
            break;
        case PLAYERSTATE_CLIMBING:
            break;
    }
}

static inline void gameroutines_playerctrl(struct gamestate *game) {
    game->entities.x[0] = game->scrollx & 0xFF00;
}

static inline void gameroutines(struct gamestate *game) {
    switch (game->gameengine_task) {
        case 0: { gameroutines_setup(game); break; }
        case 7: { gameroutines_entrace(game); break; }
        case 8: { gameroutines_playerctrl(game); break; }
    }
}

static inline void gamecore(struct gamestate *game) {


}

