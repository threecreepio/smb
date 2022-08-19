#pragma once
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif

#define MAX_ENTITY 6

#define OPERMODE_TITLE 0
#define OPERMODE_GAME 1
#define OPERMODE_VICTORY 2
#define OPERMODE_GAMEOVER 2

#define INPUT_START  0b00000001
#define INPUT_SELECT 0b00000010
#define INPUT_A      0b00000100
#define INPUT_B      0b00001000
#define INPUT_UP     0b00010000
#define INPUT_LEFT   0b00100000
#define INPUT_DOWN   0b01000000
#define INPUT_RIGHT  0b10000000


struct entities {
    uint16_t x[MAX_ENTITY];
    uint16_t y[MAX_ENTITY];
    uint8_t enabled[MAX_ENTITY];
};

struct gamestate {
    uint8_t timercontrol;
    union {
#ifdef __AVX2__
        __m256i value;
#else
        uint8_t array[0x20];
#endif
        struct {
            uint8_t itc;
            uint8_t control;
            uint8_t t02;
            uint8_t t03;
            uint8_t t04;
            uint8_t t05;
            uint8_t t06;
            uint8_t t07;
            uint8_t t08;
            uint8_t t09;
            uint8_t t0a;
            uint8_t t0b;
            uint8_t t0c;
            uint8_t t0d;
            uint8_t t0e;
            uint8_t t0f;

            uint8_t demo;
            uint8_t t11;
            uint8_t t12;
            uint8_t t13;
            uint8_t t14;
            uint8_t t15;
            uint8_t t16;
            uint8_t t17;
            uint8_t t18;
            uint8_t t19;
            uint8_t t1a;
            uint8_t t1b;
            uint8_t t1c;
            uint8_t t1d;
            uint8_t t1e;
            uint8_t t1f;
        } list;
    } timers;
    
    union {
        uint64_t value;
        uint8_t list[0x7];
    } rng;

    uint8_t framecounter;
    uint8_t joypad1;

    uint8_t pause_state;
    uint8_t pause_timer;
    bool worldselectenabled;
    int number_of_players;
    int worldnumber;
    int levelnumber;
    int opermode;
    int opermode_task;

    struct {
        uint8_t type[MAX_ENTITY];
        uint16_t x[MAX_ENTITY];
        uint16_t y[MAX_ENTITY];
    } entities;

    uint16_t scrollx;
    int destinationarea;
    int areanumber;
    int areadataoffset;
    int areaentityoffset;
};
