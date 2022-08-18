#define RAYGUI_IMPLEMENTATION
#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "raylib/raygui.h"
#include "game.h"
#include "game.c"

extern const uint8_t gameshader_bin[];

extern const uint8_t font_bin[];
extern const char fontchars_bin[];
extern const uint32_t font_bin_size;
extern const uint32_t fontchars_bin_size;

Color metatiles[0x20] = {
    { 0, 0, 0, 0xFF },
    { 10, 0, 0, 0xFF },
    { 20, 0, 0, 0xFF },
    { 30, 0, 0, 0xFF },
    { 40, 0, 0, 0xFF },
    { 50, 0, 0, 0xFF },
    { 60, 0, 0, 0xFF },
    { 70, 0, 0, 0xFF },
    { 80, 0, 0, 0xFF },
    { 90, 0, 0, 0xFF },
    { 100, 0, 0, 0xFF },
    { 110, 0, 0, 0xFF },
    { 120, 0, 0, 0xFF },
    { 130, 0, 0, 0xFF },
    { 140, 0, 0, 0xFF },
    { 150, 0, 0, 0xFF },
    { 160, 0, 0, 0xFF },
    { 170, 0, 0, 0xFF },
    { 180, 0, 0, 0xFF },
    { 190, 0, 0, 0xFF },
    { 200, 0, 0, 0xFF },
};

void joypad_read(struct gamestate *game) {
    if (!IsGamepadAvailable(0)) return;
    uint8_t left   = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)   ? INPUT_LEFT : 0;
    uint8_t right  = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)  ? INPUT_RIGHT : 0;
    uint8_t up     = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP)     ? INPUT_UP : 0;
    uint8_t down   = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)   ? INPUT_DOWN : 0;
    uint8_t b      = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)  ? INPUT_B : 0;
    uint8_t a      = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ? INPUT_A : 0;
    uint8_t select = IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT)      ? INPUT_SELECT : 0;
    uint8_t start  = IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)     ? INPUT_START : 0;
    game->joypad1 = left | right | up | down | b | a | select | start;
}


Font defaultFont = { 0 };
extern void LoadSMBFont(void)
{
    #define BIT_CHECK(a,b) ((a) & (1u << (b)))

    defaultFont.charsCount = fontchars_bin_size;
    defaultFont.charsPadding = 0;

    Image imFont = {
        .data = calloc(8*(8*defaultFont.charsCount), 2),
        .width = 8,
        .height = defaultFont.charsCount * 8,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,
        .mipmaps = 1
    };

    for (int chr = 0; chr < fontchars_bin_size; ++chr) {
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                uint8_t filled = (font_bin[(chr * 8) + y] >> x) & 1;
                ((unsigned short *)imFont.data)[x + (y * 8) + (chr * 8 * 8)] = filled ? 0xffff : 0x0000;
            }
        }
    }

    defaultFont.texture = LoadTextureFromImage(imFont);
    defaultFont.chars = (CharInfo *)RL_MALLOC(defaultFont.charsCount*sizeof(CharInfo));
    defaultFont.recs = (Rectangle *)RL_MALLOC(defaultFont.charsCount*sizeof(Rectangle));

    for (int i = 0; i < defaultFont.charsCount; i++)
    {
        defaultFont.chars[i].value = fontchars_bin[i];  // First char is 32

        defaultFont.recs[i].x = 0;
        defaultFont.recs[i].y = 8 * i;
        defaultFont.recs[i].width = 8;
        defaultFont.recs[i].height = 8;

        defaultFont.chars[i].offsetX = 0;
        defaultFont.chars[i].offsetY = 0;
        defaultFont.chars[i].advanceX = 0;

        defaultFont.chars[i].image = ImageFromImage(imFont, defaultFont.recs[i]);
    }

    defaultFont.baseSize = 8;
}

static inline int main_gui(int argc, char **argv) {
    int scale = 3;
    const int screenWidth = 256 * scale;
    const int screenHeight = 240 * scale;

    InitWindow(screenWidth, screenHeight, "Super Mario C");
    Image imBlank = GenImageColor(1024, 1024, BLANK);
    Texture2D texture = LoadTextureFromImage(imBlank);  // Load blank texture to fill on shader
    UnloadImage(imBlank);

    LoadSMBFont();

    struct gamestate game;
    memset(&game, 0, sizeof(struct gamestate));

printf("%s\n", gameshader_bin);
    Shader shader = LoadShaderFromMemory(0, &gameshader_bin);

    float time = 0.0f;
    int timeLoc = GetShaderLocation(shader, "uTime");
    SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

    SetTargetFPS(60);
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        time = (float)GetTime();
        SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(CLITERAL(Color){ 146, 144, 255, 255 });
        BeginShaderMode(shader);
            DrawTexture(texture, 0, 0, WHITE);
        EndShaderMode();
        EndDrawing();

/*
        for (int bgy = 0; bgy < 0x10; ++bgy) {
            for (int bgx = 0; bgx <= 0x10; ++bgx) {
                uint8_t metatile = areadata[0][bgx + (game.scrollx >> 4)][bgy];
                if (metatile == 0) continue;
                Color bg = metatiles[metatile % 0x20];
                int scrollx_px = game.scrollx & 0x0F;
                DrawRectangle((0x10 * bgx - scrollx_px) * scale, (0x10 * bgy) * scale, 0x10 * scale, 0x10 * scale, bg);
            }
        }

        Vector2 fp = { 0x8 * scale, 0x8 * scale };
        DrawTextEx(defaultFont, "MARIO", fp, 0x8 * scale, 0.0f, WHITE);
        fp.y += 0x8 * scale;
        DrawTextEx(defaultFont, "000000", fp, 0x8 * scale, 0.0f, WHITE);
        EndDrawing();
        joypad_read(&game);
        if (game.joypad1 & INPUT_RIGHT) game.scrollx += 1;
        else if (game.joypad1 & INPUT_LEFT) game.scrollx -= 1;
        nmi(&game);
        */
    }

    UnloadShader(shader);
    CloseWindow();
    return 0;
}

static inline int main_headless(int argc, char **argv) {
    struct gamestate game;
    memset(&game, 0, sizeof(struct gamestate));
    double start = get_millis();
    #define COUNT 100000000

    for (int i=0; i<COUNT; ++i) {
        nmi(&game);
    }
    
    double end = get_millis();

    printf("size: %lld\n", sizeof(struct gamestate));
    printf("ms elapsed: %f\n", (end - start));
    printf("ms per step: %f\n", (end - start) / ((float)COUNT));
    printf("fps: %f\n", 1000.0 / ((end - start) / ((float)COUNT)));
    return 0;
}

int main(int argc, char **argv) {
    return main_gui(argc, argv);
}
