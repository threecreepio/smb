#include "game.h"

static inline void gameroutines_setup(struct gamestate *game) {
    //game->entities.x[0] = game->scrollx & 0xFF00;
    game->gameengine_task = 7;
}

static inline void gameroutines_entrance(struct gamestate *game) {
    game->gameengine_task = 8;
}

static inline void gameroutines_playerphysics(struct gamestate *game) {
    if (game->player.state == PLAYERSTATE_CLIMBING) {
        // check player climb direction
        uint8_t dir = (INPUT_UP | INPUT_DOWN) | (game->joypad1 & game->entities.collision[0]);
        if (dir & INPUT_UP) game->entities.yspeed[0] = -0xe000;
        else if (dir & INPUT_DOWN) game->entities.yspeed[0] = 0x1f00;
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
            game->entities.yspeed[0] &= 0xFF00;
            // and set our current position as the origin of the jump
            game->player.jumporigin_y = game->entities.y[0];
            if (game->player.swimming) {
                // play swimming sound
                sound_play(game, SFX_ENEMYSTOMP);
                // get swim movement based on whirlpool state
                if (game->player.inwhirlpool) {
                    game->player.verticalforce = 0x04;
                    game->player.verticalforcedown = 0x09;
                    game->entities.yspeed[0] = -0x1000;
                } else {
                    game->player.verticalforce = 0x0D;
                    game->player.verticalforcedown = 0x0A;
                    game->entities.yspeed[0] = -0x1800;
                }
                // if player is too close to the top of the screen, cancel upward movement
                if (game->entities.y[0] <= 0x1400) {
                    game->entities.yspeed[0] &= 0x0F;
                }
            } else {
                // play normal jumping sound
                sound_play(game, game->player.size ? SFX_JUMP_SMALL : SFX_JUMP_BIG);
                // and get movement forces based on players movement speed
                int16_t speed = game->player.xspeed_absolute;
                if (speed >= 0x1900) {
                    game->player.verticalforce = 0x28;
                    game->player.verticalforcedown = 0x90;
                    game->entities.yspeed[0] = 0xFB00;
                } else if (speed >= 0x1000) {
                    game->player.verticalforce = 0x1E;
                    game->player.verticalforcedown = 0x60;
                    game->entities.yspeed[0] = 0xFC00;
                } else {
                    game->player.verticalforce = 0x20;
                    game->player.verticalforcedown = 0x70;
                    game->entities.yspeed[0] = 0xFC00;
                }
            }
        }
    }

    // x physics
    int gottagofast = 0;
    int speedvalue = 0;
    int16_t maxspeed = 0x1000;
    int16_t friction = 0xd0;
    if (game->player.state != PLAYERSTATE_ONGROUND) {
        gottagofast = game->player.xspeed_absolute <= 0x1900;
    } else if (game->player.swimming) {
        gottagofast = 1;
    } else if ((game->joypad1 & game->player.movingdir) == 0) {
        gottagofast = 1;
    } else {
        maxspeed = 0x1800;
        friction = 0x98;
        gottagofast = game->joypad1 & INPUT_B;
    }

    if (gottagofast) {
        if (game->gameengine_task == 0x7) {
            maxspeed = 0x1000;
            friction = 0xd0;
        } else if (game->player.runningspeed || game->player.xspeed_absolute >= 0x2100) {
            speedvalue += 1;
            maxspeed = 0x2800;
            friction = 0xe4;
        } else {
            maxspeed = 0x1800;
            friction = 0x98;
        }
    }

    game->player.maxleftspeed = -maxspeed;
    game->player.maxrightspeed = maxspeed;
    game->player.friction = friction;
    game->player.movingdir = game->entities.xspeed[0] <= 0 ? INPUT_LEFT : INPUT_RIGHT;

    // double friction value if moving against facing direction
    if (game->player.movingdir != game->player.facing) {
        game->player.friction *= 2;
    }
}

static inline void imposegravity(struct gamestate *game, uint8_t entity, int8_t force, int16_t maxspeed) {
    game->entities.y[entity] += game->entities.yspeed[entity];
    game->entities.yspeed[entity] += force;
    if (game->entities.yspeed[entity] > maxspeed && (game->entities.yspeed[entity] & 0xFF) >= 0x80) {
        game->entities.yspeed[entity] = maxspeed;
    }
}

static inline void player_friction(struct gamestate *game) {
    uint8_t lrinputs = game->joypad1 & (INPUT_LEFT | INPUT_RIGHT);
    int16_t xspeed = game->entities.xspeed[0];
    int friction_dir = game->player.movingdir == INPUT_LEFT ? INPUT_RIGHT : INPUT_LEFT;
    //if ((lrinputs & game->entities.collision[0]) == 0) {
    if ((lrinputs & 0xFF) == 0) {
        if ((xspeed & 0xFF00) == 0) {
            game->player.xspeed_absolute = 0;
            return;
        }
        if (xspeed > 0) friction_dir = -1;
        else {
            printf("LLLL\n");
            friction_dir = 1;
        }
    } else {
        friction_dir = lrinputs & INPUT_LEFT ? -1 : 1;
        //if () friction_dir = -1;
        //else friction_dir = 1;
    }
    if (friction_dir == 1) {
        xspeed += game->player.friction;
        if ((int16_t)(xspeed & 0xFF00) > game->player.maxrightspeed) {
            printf("MAXSPEED %04X (max is %04X)\n", xspeed, game->player.maxrightspeed);
            xspeed = game->player.maxrightspeed | (xspeed & 0xFF);
        }
        game->player.xspeed_absolute = xspeed;
    } else if (friction_dir == -1) {
        xspeed -= game->player.friction;
        if ((int16_t)(xspeed & 0xFF00) < game->player.maxleftspeed) {
            xspeed = game->player.maxleftspeed | (xspeed & 0xFF);
        }
        game->player.xspeed_absolute = -xspeed;
    }
    game->entities.xspeed[0] = xspeed;
}

static inline void player_movehorizontal(struct gamestate *game) {
    game->entities.x[0] = (game->entities.x[0] + (game->entities.xspeed[0] >> 4)) & 0xFFFFF0;
    game->player.x_scroll = game->entities.xspeed[0]; // & 0xFF;
    if ((game->entities.x[0] - game->scrollx) >= 0x7000) {
        //game->scrollx += 0x0200;
        game->scrollx += game->player.x_scroll >> 4;
    }
}

static inline void player_movevertical(struct gamestate *game) {
    if (game->timercontrol == 0 && game->jumpspring_state != 0) return;
    imposegravity(game, 0, game->player.verticalforce, 0x400);
}


static inline void gameroutines_player_onground(struct gamestate *game) {
    uint8_t lrinputs = game->joypad1 & (INPUT_LEFT | INPUT_RIGHT);
    if (lrinputs) game->player.facing = lrinputs;
    player_friction(game);
    player_movehorizontal(game);
}

static inline void gameroutines_player_jumpswim(struct gamestate *game) {
    if (
        game->entities.yspeed[0] >= 0
        || (game->joypad1 & game->joypad1_previous & INPUT_A) == 0
        || (game->player.jumporigin_y - game->entities.y[0]) < 0x1
    ) {
        game->player.verticalforce = game->player.verticalforcedown;
    }
    uint8_t directioninput = game->joypad1 & (INPUT_LEFT | INPUT_RIGHT);
    if (game->player.swimming) {
        // GetPlayerAnimSpeed()
        if (game->entities.y[0] <= 0x1400) {
            game->player.verticalforce = 0x18;
        }
        if (directioninput) {
            game->player.facing = directioninput;
        }
    }
    if (directioninput) {
        player_friction(game);
    }
    player_movehorizontal(game);
    if (game->gameengine_task == 0x0b) {
        game->player.verticalforce = 0x28;
    }
    player_movevertical(game);

    // temp
    if (game->entities.y[0] >= 0x01B000) {
        game->entities.y[0] = 0x01B000;
        game->player.state = PLAYERSTATE_ONGROUND;
    }
}

static inline void gameroutines_playermovement(struct gamestate *game) {
    // if the player is large and holding the down button while on land, set crouch flag
    if (game->player.size == PLAYERSIZE_SMALL) {
        game->player.crouching = 0;
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
        case PLAYERSTATE_ONGROUND: {
            gameroutines_player_onground(game);
            break;
        }
        case PLAYERSTATE_JUMPSWIM: {
            gameroutines_player_jumpswim(game);
            break;
        }
        case PLAYERSTATE_FALLING:
            break;
        case PLAYERSTATE_CLIMBING:
            break;
    }
}

static inline void gameroutines_playerctrl(struct gamestate *game) {
    gameroutines_playermovement(game);
}

static inline void gameroutines(struct gamestate *game) {
    switch (game->gameengine_task) {
        case 0: { gameroutines_setup(game); break; }
        case 7: { gameroutines_entrance(game); break; }
        case 8: { gameroutines_playerctrl(game); break; }
    }
}

static inline void gamecore(struct gamestate *game) {
    gameroutines(game);


}

