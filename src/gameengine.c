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
        if (dir & INPUT_UP) game->entities.yspeed[0] = 0xe000;
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
                    game->entities.yspeed[0] = 0xFF00;
                } else {
                    game->player.verticalforce = 0x0D;
                    game->player.verticalforcedown = 0x0A;
                    game->entities.yspeed[0] = 0xFE80;
                }
                // if player is too close to the top of the screen, cancel upward movement
                if (game->entities.y[0] <= 0x1400) {
                    game->entities.yspeed[0] &= 0xFF;
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
    // set default slow-walk speed
    int16_t maxspeed = 0x1000, friction = 0xd0;
    int allowfullspeed = 0;
    if (game->gameengine_task != 0x7) {
        if (game->player.state != PLAYERSTATE_ONGROUND) {
            // if we're not on the ground
            // we can advance to full running speed as long as we stay above $19 speed
            allowfullspeed = game->player.xspeed_absolute >= 0x1900;
        } else if (game->player.swimming) {
            // we can always move at full speed when swimming
            allowfullspeed = 1;
        } else if ((game->joypad1 & game->player.movingdir) == 0) {
            // we can move at full speed if not holding the direction we're moving in
            allowfullspeed = 1;
        } else if ((game->joypad1 & INPUT_B) == 0) {
            // if not holding B, we can keep running while:
            //  - the running timer is still active
            //  - the running speed animation is running
            //  - the player is moving at more than 2.1px per frame
            allowfullspeed = game->timers.list.running != 0 ||
                game->player.runningspeed ||
                game->player.xspeed_absolute >= 0x2100
            ;
            //maxspeed = 0x1800;
            //friction = 0x98;
            //gottagofast = game->joypad1 & INPUT_B;
            //if (game->joypad1 & INPUT_B) {
            //    game->timers.list.running = 0x0a;
            //}
            //printf("%02X\n", game->joypad1 & INPUT_B);
        } else {
            // otherwise while holding B we can move at full speed
            allowfullspeed = 1;
            // also set the runspeed countdown to use when releasing B
            game->timers.list.running = 0xA;
        }

        maxspeed = 0x1800;
        friction = 0x98;
        if (allowfullspeed) {
            //} else if (game->player.runningspeed || game->player.xspeed_absolute >= 0x2100) {
            //if (game->player.runningspeed || game->player.xspeed_absolute >= 0x2100) {
                maxspeed = 0x2800;
                friction = 0xe4;
            //}
        }
    }

    game->player.maxleftspeed = -maxspeed;
    game->player.maxrightspeed = maxspeed;
    game->player.friction = friction;

    // double friction value if moving against facing direction
    if (game->player.movingdir != game->player.facing) {
        game->player.friction *= 2;
    }
}

static inline void imposegravity(struct gamestate *game, uint8_t entity, uint8_t force, int16_t maxspeed) {
    game->entities.y[entity] += game->entities.yspeed[entity];
    game->entities.yspeed[entity] += force;
    if (game->entities.yspeed[entity] > maxspeed && (game->entities.yspeed[entity] & 0xFF) >= 0x80) {
        game->entities.yspeed[entity] = maxspeed;
    }
}

static inline void player_friction(struct gamestate *game) {
    uint8_t lrinputs = game->joypad1 & (INPUT_LEFT | INPUT_RIGHT);
    int16_t xspeed = game->entities.xspeed[0];
    int16_t xspeed_whole = xspeed & 0xFF00;
    int friction_dir = 0;
    // todo - AND with collision bits
    if ((lrinputs & game->entities.collision[0]) == 0) {
        if (xspeed_whole == 0) {
            game->player.xspeed_absolute = 0;
            return;
        }
        if (xspeed_whole >= 0) friction_dir = -1;
        else friction_dir = 1;
    } else {
        friction_dir = lrinputs & INPUT_RIGHT ? 1 : -1;
        //if () friction_dir = -1;
        //else friction_dir = 1;
    }
    if (friction_dir == 1) {
        xspeed += game->player.friction;
        xspeed_whole = xspeed & 0xFF00;
        if (xspeed_whole > game->player.maxrightspeed) {
            xspeed = game->player.maxrightspeed | (xspeed & 0xFF);
        }
        game->player.xspeed_absolute = xspeed;
    } else if (friction_dir == -1) {
        xspeed -= game->player.friction;
        xspeed_whole = xspeed & 0xFF00;
        if (xspeed_whole < game->player.maxleftspeed) {
            xspeed = game->player.maxleftspeed | (xspeed & 0xFF);
        }
        game->player.xspeed_absolute = -xspeed;
    }
    game->entities.xspeed[0] = xspeed;
}

static inline void player_movehorizontal(struct gamestate *game) {
    if (game->jumpspring_state != 0) return;
    int32_t prev_x = game->entities.x[0];
    int32_t next_x = (prev_x + (game->entities.xspeed[0] >> 4)) & 0xFFFFF0;
    game->player.x_scroll = (next_x - prev_x) >> 7;
    //printf("player_x_scroll: %d\n", game->player.x_scroll);

    game->entities.x[0] = next_x;
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
    uint8_t directioninput = game->joypad1 & (INPUT_LEFT | INPUT_RIGHT);
    // check so this isn't the first frame of our jump
    if (game->timers.list.jumpswim < 0x20) {
        // and we've either come to a stop or have released A
        if (!(game->entities.yspeed[0] & 0xFF00) || !(game->joypad1 & game->joypad1_previous & INPUT_A)) {
            // if so, change to falling force.
            game->player.verticalforce = game->player.verticalforcedown;
        }
    }
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
    //if (game->entities.y[0] >= 0x01B000) {
    //    game->entities.y[0] = 0x01B000;
    //    game->player.state = PLAYERSTATE_ONGROUND;
    //}
}

static inline void playerstate_falling(struct gamestate *game) {
    game->player.verticalforce = game->player.verticalforcedown;
    if (game->joypad1 & (INPUT_LEFT | INPUT_RIGHT)) {
        player_friction(game);
    }
    player_movehorizontal(game);
    if (game->gameengine_task == 0xb) {
        game->player.verticalforce = 0x28;
    }
    player_movevertical(game);
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
            playerstate_falling(game);
            break;
        case PLAYERSTATE_CLIMBING:
            break;
    }
}


#define MTILEFLAG_NONE 0
#define MTILEFLAG_SOLID 1
#define MTILEFLAG_CLIMB 2

uint8_t metatile_flags[0x100] = {
    MTILEFLAG_NONE, // 0x00
    MTILEFLAG_NONE, // 0x01
    MTILEFLAG_NONE, // 0x02
    MTILEFLAG_NONE, // 0x03
    MTILEFLAG_NONE, // 0x04
    MTILEFLAG_NONE, // 0x05
    MTILEFLAG_NONE, // 0x06
    MTILEFLAG_NONE, // 0x07
    MTILEFLAG_NONE, // 0x08
    MTILEFLAG_NONE, // 0x09
    MTILEFLAG_NONE, // 0x0A
    MTILEFLAG_NONE, // 0x0B
    MTILEFLAG_NONE, // 0x0C
    MTILEFLAG_NONE, // 0x0D
    MTILEFLAG_NONE, // 0x0E
    MTILEFLAG_NONE, // 0x0F
    
    MTILEFLAG_SOLID, // 0x10
    MTILEFLAG_SOLID, // 0x11
    MTILEFLAG_SOLID, // 0x12
    MTILEFLAG_SOLID, // 0x13
    MTILEFLAG_SOLID, // 0x14
    MTILEFLAG_SOLID, // 0x15
    MTILEFLAG_NONE, // 0x16
    MTILEFLAG_NONE, // 0x17
    MTILEFLAG_NONE, // 0x18
    MTILEFLAG_NONE, // 0x19
    MTILEFLAG_NONE, // 0x1A
    MTILEFLAG_NONE, // 0x1B
    MTILEFLAG_NONE, // 0x1C
    MTILEFLAG_NONE, // 0x1D
    MTILEFLAG_NONE, // 0x1E
    MTILEFLAG_NONE, // 0x1F

    MTILEFLAG_NONE, // 0x20
    MTILEFLAG_NONE, // 0x21
    MTILEFLAG_NONE, // 0x22
    MTILEFLAG_NONE, // 0x23
    MTILEFLAG_NONE, // 0x24
    MTILEFLAG_NONE, // 0x25
    MTILEFLAG_NONE, // 0x26
    MTILEFLAG_NONE, // 0x27
    MTILEFLAG_NONE, // 0x28
    MTILEFLAG_NONE, // 0x29
    MTILEFLAG_NONE, // 0x2A
    MTILEFLAG_NONE, // 0x2B
    MTILEFLAG_NONE, // 0x2C
    MTILEFLAG_NONE, // 0x2D
    MTILEFLAG_NONE, // 0x2E
    MTILEFLAG_NONE, // 0x2F
    
    MTILEFLAG_NONE, // 0x30
    MTILEFLAG_NONE, // 0x31
    MTILEFLAG_NONE, // 0x32
    MTILEFLAG_NONE, // 0x33
    MTILEFLAG_NONE, // 0x34
    MTILEFLAG_NONE, // 0x35
    MTILEFLAG_NONE, // 0x36
    MTILEFLAG_NONE, // 0x37
    MTILEFLAG_NONE, // 0x38
    MTILEFLAG_NONE, // 0x39
    MTILEFLAG_NONE, // 0x3A
    MTILEFLAG_NONE, // 0x3B
    MTILEFLAG_NONE, // 0x3C
    MTILEFLAG_NONE, // 0x3D
    MTILEFLAG_NONE, // 0x3E
    MTILEFLAG_NONE, // 0x3F

    MTILEFLAG_NONE, // 0x40
    MTILEFLAG_NONE, // 0x41
    MTILEFLAG_NONE, // 0x42
    MTILEFLAG_NONE, // 0x43
    MTILEFLAG_NONE, // 0x44
    MTILEFLAG_NONE, // 0x45
    MTILEFLAG_NONE, // 0x46
    MTILEFLAG_NONE, // 0x47
    MTILEFLAG_NONE, // 0x48
    MTILEFLAG_NONE, // 0x49
    MTILEFLAG_NONE, // 0x4A
    MTILEFLAG_NONE, // 0x4B
    MTILEFLAG_NONE, // 0x4C
    MTILEFLAG_NONE, // 0x4D
    MTILEFLAG_NONE, // 0x4E
    MTILEFLAG_NONE, // 0x4F

    MTILEFLAG_NONE, // 0x50
    MTILEFLAG_SOLID, // 0x51
    MTILEFLAG_NONE, // 0x52
    MTILEFLAG_NONE, // 0x53
    MTILEFLAG_SOLID, // 0x54
    MTILEFLAG_NONE, // 0x55
    MTILEFLAG_NONE, // 0x56
    MTILEFLAG_NONE, // 0x57
    MTILEFLAG_NONE, // 0x58
    MTILEFLAG_NONE, // 0x59
    MTILEFLAG_NONE, // 0x5A
    MTILEFLAG_NONE, // 0x5B
    MTILEFLAG_NONE, // 0x5C
    MTILEFLAG_NONE, // 0x5D
    MTILEFLAG_NONE, // 0x5E
    MTILEFLAG_NONE, // 0x5F

    MTILEFLAG_NONE, // 0x60
    MTILEFLAG_SOLID, // 0x61
    MTILEFLAG_NONE, // 0x62
    MTILEFLAG_NONE, // 0x63
    MTILEFLAG_NONE, // 0x64
    MTILEFLAG_NONE, // 0x65
    MTILEFLAG_NONE, // 0x66
    MTILEFLAG_NONE, // 0x67
    MTILEFLAG_NONE, // 0x68
    MTILEFLAG_NONE, // 0x69
    MTILEFLAG_NONE, // 0x6A
    MTILEFLAG_NONE, // 0x6B
    MTILEFLAG_NONE, // 0x6C
    MTILEFLAG_NONE, // 0x6D
    MTILEFLAG_NONE, // 0x6E
    MTILEFLAG_NONE, // 0x6F

    MTILEFLAG_NONE, // 0x70
    MTILEFLAG_NONE, // 0x71
    MTILEFLAG_NONE, // 0x72
    MTILEFLAG_NONE, // 0x73
    MTILEFLAG_NONE, // 0x74
    MTILEFLAG_NONE, // 0x75
    MTILEFLAG_NONE, // 0x76
    MTILEFLAG_NONE, // 0x77
    MTILEFLAG_NONE, // 0x78
    MTILEFLAG_NONE, // 0x79
    MTILEFLAG_NONE, // 0x7A
    MTILEFLAG_NONE, // 0x7B
    MTILEFLAG_NONE, // 0x7C
    MTILEFLAG_NONE, // 0x7D
    MTILEFLAG_NONE, // 0x7E
    MTILEFLAG_NONE, // 0x7F

    MTILEFLAG_NONE, // 0x80
    MTILEFLAG_NONE, // 0x81
    MTILEFLAG_NONE, // 0x82
    MTILEFLAG_NONE, // 0x83
    MTILEFLAG_NONE, // 0x84
    MTILEFLAG_NONE, // 0x85
    MTILEFLAG_NONE, // 0x86
    MTILEFLAG_NONE, // 0x87
    MTILEFLAG_NONE, // 0x88
    MTILEFLAG_NONE, // 0x89
    MTILEFLAG_NONE, // 0x8A
    MTILEFLAG_NONE, // 0x8B
    MTILEFLAG_NONE, // 0x8C
    MTILEFLAG_NONE, // 0x8D
    MTILEFLAG_NONE, // 0x8E
    MTILEFLAG_NONE, // 0x8F

    MTILEFLAG_NONE, // 0x90
    MTILEFLAG_NONE, // 0x91
    MTILEFLAG_NONE, // 0x92
    MTILEFLAG_NONE, // 0x93
    MTILEFLAG_NONE, // 0x94
    MTILEFLAG_NONE, // 0x95
    MTILEFLAG_NONE, // 0x96
    MTILEFLAG_NONE, // 0x97
    MTILEFLAG_NONE, // 0x98
    MTILEFLAG_NONE, // 0x99
    MTILEFLAG_NONE, // 0x9A
    MTILEFLAG_NONE, // 0x9B
    MTILEFLAG_NONE, // 0x9C
    MTILEFLAG_NONE, // 0x9D
    MTILEFLAG_NONE, // 0x9E
    MTILEFLAG_NONE, // 0x9F

    MTILEFLAG_NONE, // 0xA0
    MTILEFLAG_NONE, // 0xA1
    MTILEFLAG_NONE, // 0xA2
    MTILEFLAG_NONE, // 0xA3
    MTILEFLAG_NONE, // 0xA4
    MTILEFLAG_NONE, // 0xA5
    MTILEFLAG_NONE, // 0xA6
    MTILEFLAG_NONE, // 0xA7
    MTILEFLAG_NONE, // 0xA8
    MTILEFLAG_NONE, // 0xA9
    MTILEFLAG_NONE, // 0xAA
    MTILEFLAG_NONE, // 0xAB
    MTILEFLAG_NONE, // 0xAC
    MTILEFLAG_NONE, // 0xAD
    MTILEFLAG_NONE, // 0xAE
    MTILEFLAG_NONE, // 0xAF

    MTILEFLAG_NONE, // 0xB0
    MTILEFLAG_NONE, // 0xB1
    MTILEFLAG_NONE, // 0xB2
    MTILEFLAG_NONE, // 0xB3
    MTILEFLAG_NONE, // 0xB4
    MTILEFLAG_NONE, // 0xB5
    MTILEFLAG_NONE, // 0xB6
    MTILEFLAG_NONE, // 0xB7
    MTILEFLAG_NONE, // 0xB8
    MTILEFLAG_NONE, // 0xB9
    MTILEFLAG_NONE, // 0xBA
    MTILEFLAG_NONE, // 0xBB
    MTILEFLAG_NONE, // 0xBC
    MTILEFLAG_NONE, // 0xBD
    MTILEFLAG_NONE, // 0xBE
    MTILEFLAG_NONE, // 0xBF

    MTILEFLAG_SOLID, // 0xC0
    MTILEFLAG_SOLID, // 0xC1
    MTILEFLAG_NONE, // 0xC2
    MTILEFLAG_NONE, // 0xC3
    MTILEFLAG_NONE, // 0xC4
    MTILEFLAG_NONE, // 0xC5
    MTILEFLAG_NONE, // 0xC6
    MTILEFLAG_NONE, // 0xC7
    MTILEFLAG_NONE, // 0xC8
    MTILEFLAG_NONE, // 0xC9
    MTILEFLAG_NONE, // 0xCA
    MTILEFLAG_NONE, // 0xCB
    MTILEFLAG_NONE, // 0xCC
    MTILEFLAG_NONE, // 0xCD
    MTILEFLAG_NONE, // 0xCE
    MTILEFLAG_NONE, // 0xCF

    MTILEFLAG_NONE, // 0xD0
    MTILEFLAG_NONE, // 0xD1
    MTILEFLAG_NONE, // 0xD2
    MTILEFLAG_NONE, // 0xD3
    MTILEFLAG_NONE, // 0xD4
    MTILEFLAG_NONE, // 0xD5
    MTILEFLAG_NONE, // 0xD6
    MTILEFLAG_NONE, // 0xD7
    MTILEFLAG_NONE, // 0xD8
    MTILEFLAG_NONE, // 0xD9
    MTILEFLAG_NONE, // 0xDA
    MTILEFLAG_NONE, // 0xDB
    MTILEFLAG_NONE, // 0xDC
    MTILEFLAG_NONE, // 0xDD
    MTILEFLAG_NONE, // 0xDE
    MTILEFLAG_NONE, // 0xDF

    MTILEFLAG_NONE, // 0xE0
    MTILEFLAG_NONE, // 0xE1
    MTILEFLAG_NONE, // 0xE2
    MTILEFLAG_NONE, // 0xE3
    MTILEFLAG_NONE, // 0xE4
    MTILEFLAG_NONE, // 0xE5
    MTILEFLAG_NONE, // 0xE6
    MTILEFLAG_NONE, // 0xE7
    MTILEFLAG_NONE, // 0xE8
    MTILEFLAG_NONE, // 0xE9
    MTILEFLAG_NONE, // 0xEA
    MTILEFLAG_NONE, // 0xEB
    MTILEFLAG_NONE, // 0xEC
    MTILEFLAG_NONE, // 0xED
    MTILEFLAG_NONE, // 0xEE
    MTILEFLAG_NONE, // 0xEF

    MTILEFLAG_NONE, // 0xF0
    MTILEFLAG_NONE, // 0xF1
    MTILEFLAG_NONE, // 0xF2
    MTILEFLAG_NONE, // 0xF3
    MTILEFLAG_NONE, // 0xF4
    MTILEFLAG_NONE, // 0xF5
    MTILEFLAG_NONE, // 0xF6
    MTILEFLAG_NONE, // 0xF7
    MTILEFLAG_NONE, // 0xF8
    MTILEFLAG_NONE, // 0xF9
    MTILEFLAG_NONE, // 0xFA
    MTILEFLAG_NONE, // 0xFB
    MTILEFLAG_NONE, // 0xFC
    MTILEFLAG_NONE, // 0xFD
    MTILEFLAG_NONE, // 0xFE
    MTILEFLAG_NONE, // 0xFF
};


static inline void player_bgcollision(struct gamestate *game) {
    //if (game->player.disablecollisions || game->gameengine_task == 0xb)

    game->entities.collision[0] = 0xFF;

    if (game->entities.yspeed[0] < 0x0) {
        // head check
        uint16_t x = ((game->entities.x[0] + 0x0800) >> 12);
        uint8_t  y = ((game->entities.y[0] + 0x1200) >> 12) & 0xF;
        uint8_t block = game->areadata[x][1+y];
        if (metatile_flags[block] & MTILEFLAG_SOLID) {
            printf("head bonk @ %06X\n", game->entities.y[0]);
            game->entities.yspeed[0] = 0x0100;
            //game->player.state = PLAYERSTATE_ONGROUND;
            //game->entities.y[0] &= 0xFFFFF000;
            //game->player.state = PLAYERSTATE_FALLING;
        }
    } else if ((game->entities.y[0] & 0xFF00) <= 0xCF00) {
        // foot check
        uint16_t left_x  = ((game->entities.x[0] + 0x0300) >> 12);
        uint16_t right_x = ((game->entities.x[0] + 0x0C00) >> 12);
        uint8_t  y       = ((game->entities.y[0] + 0x2000) >> 12) & 0xF;
        uint8_t left_block = game->areadata[left_x][1+y];
        uint8_t right_block = game->areadata[right_x][1+y];

        if (metatile_flags[right_block] | metatile_flags[left_block] & MTILEFLAG_SOLID) {
            game->player.state = PLAYERSTATE_ONGROUND;
            game->entities.y[0] &= 0xFFFFF000;
        } else {
            game->player.state = PLAYERSTATE_FALLING;
        }
    }

    // side check
    uint8_t sidehit = 0;
    uint8_t side = 0;
    do {
        // if we're falling off the screen, no need to do hit test
        if ((game->entities.y[0] & 0xFF00) >= 0xE400) break;
        uint16_t left_x = ((game->entities.x[0] + 0x0200) >> 12);
        uint8_t high_y = ((game->entities.y[0] + 0x0800) >> 12) & 0xF;
        uint8_t low_y = ((game->entities.y[0] + 0x1800) >> 12) & 0xF;
        if (game->player.crouching || game->player.size == PLAYERSIZE_SMALL) {
            high_y = low_y;
        }
        uint8_t high_left_block = game->areadata[left_x][1+high_y];

        // check upper left side hit
        uint8_t test_high_left = (game->entities.y[0] & 0xFF00) < 0x2000 ? 0 : (
            (high_left_block == 0x1c || high_left_block == 0x6b) ? 0 : metatile_flags[high_left_block]
        ) & (0xFF ^ MTILEFLAG_CLIMB);

        // if we hit here, exit with that tile
        if (test_high_left) {
            side = 0;
            sidehit = high_left_block;
            break;
        }

        // if we hit a tile or we're outside the bounds, bail out
        if ((game->entities.y[0] & 0xFF00) < 0x0800 || (game->entities.y[0] & 0xFF00) >= 0xD000) break;
        uint16_t right_x = ((game->entities.x[0] + 0x0D00) >> 12);
        uint8_t high_right_block = game->areadata[right_x][1+high_y];

        // if we hit any tile on the upper right, exit with that tile
        if (metatile_flags[high_right_block]) {
            side = 1;
            sidehit = high_right_block;
            break;
        }

        uint8_t low_left_block = game->areadata[left_x][1+low_y];

        // check upper left side hit rules
        uint8_t test_low_left = (game->entities.y[0] & 0xFF00) < 0x2000 ? 0 : (
            (low_left_block == 0x1c || low_left_block == 0x6b) ? 0 : metatile_flags[low_left_block]
        ) & (0xFF ^ MTILEFLAG_CLIMB);

        // if we hit here, exit with that tile
        if (test_low_left) {
            side = 0;
            sidehit = low_left_block;
            break;
        }

        uint8_t low_right_block = game->areadata[right_x][1+low_y];
        // if we hit any tile on the upper right, exit with that tile
        if (metatile_flags[low_right_block]) {
            side = 1;
            sidehit = low_right_block;
            break;
        }
    } while (0);

    if (sidehit) {
        uint8_t hitdir = side ? INPUT_RIGHT : INPUT_LEFT;
        int16_t adjust_position = hitdir == INPUT_RIGHT ? -0x0100 : 0x0100;
        if (metatile_flags[sidehit] & MTILEFLAG_SOLID) {
            game->entities.xspeed[0] &= 0xFF;
            //game->entities.x[0] = (game->entities.x[0] & 0xFFFFF000) + adjust_position;
            game->entities.x[0] = game->entities.x[0] + adjust_position;
            game->entities.collision[0] = 0xFF ^ hitdir;
            printf(":: %02X\n", game->entities.collision[0]);
        }
    }

}

static inline void gameroutines_playerctrl(struct gamestate *game) {
    // if pressing down while on ground, prevent directional movement
    if (game->player.state == PLAYERSTATE_ONGROUND && (game->joypad1 & INPUT_DOWN)) {
        game->joypad1 &= 0xFF ^ INPUT_LEFT ^ INPUT_RIGHT;
    }
    gameroutines_playermovement(game);
    game->entities.bboxtype[0] = (game->player.size == PLAYERSIZE_SMALL || game->player.crouching) ? BBOX_PLAYER_SMALL : BBOX_PLAYER_BIG;
    // set movement direction value based on speed
    uint16_t xspeed = game->entities.xspeed[0] & 0xFF00;
    if (xspeed > 0) game->player.movingdir = INPUT_RIGHT;
    else if (xspeed < 0) game->player.movingdir = INPUT_LEFT;
    // ScrollHandler();
    player_bgcollision(game);


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

