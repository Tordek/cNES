#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "nes_header.h"
#include "ic_rp2a03.h"
#include "controllers.h"
#include "mapper.h"
#include "mappers.h"
#include "ic_6502.h"
#include "ic_2c02.h"

#include "debug.h"

static void ic_rp2a03_sdl_audio_callback(void *userdata, uint8_t *stream, int len);

static int32_t const palette_table[128] = {
    0xff7c7c7c, 0xff0000fc, 0xff0000bc, 0xff4428bc, 0xff940084, 0xffa80020, 0xffa81000, 0xff881400, 0xff503000, 0xff007800, 0xff006800, 0xff005800, 0xff004058, 0xff000000, 0xff000000, 0xff000000,
    0xffbcbcbc, 0xff0078f8, 0xff0058f8, 0xff6844fc, 0xffd800cc, 0xffe40058, 0xfff83800, 0xffe45c10, 0xffac7c00, 0xff00b800, 0xff00a800, 0xff00a844, 0xff008888, 0xff000000, 0xff000000, 0xff000000,
    0xfff8f8f8, 0xff3cbcfc, 0xff6888fc, 0xff9878f8, 0xfff878f8, 0xfff85898, 0xfff87858, 0xfffca044, 0xfff8b800, 0xffb8f818, 0xff58d854, 0xff58f898, 0xff00e8d8, 0xff787878, 0xff000000, 0xff000000,
    0xfffcfcfc, 0xffa4e4fc, 0xffb8b8f8, 0xffd8b8f8, 0xfff8b8f8, 0xfff8a4c0, 0xfff0d0b0, 0xfffce0a8, 0xfff8d878, 0xffd8f878, 0xffb8f8b8, 0xffb8f8d8, 0xff00fcfc, 0xfff8d8f8, 0xff000000, 0xff000000,
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s rom\n", argv[0]);
        return -1;
    }

    struct nes_rom rom;
    int rc = read_rom(&rom, argv[1]);

    if (rc) {
        printf("Not a iNES/NES2.0 file");
        return rc;
    }

    printf("ROM info: %s\n", argv[1]);
    printf("Mapper: %d\n", rom.mapper_id);
    printf("PRG_ROM size: %d\n", rom.prg_rom_size * 0x4000);
    printf("CHR_ROM size: %d\n", rom.chr_rom_size * 0x2000);

    struct mapper *mapper = (mappers[rom.mapper_id])(&rom);

    struct ic_6502_registers cpu;
    struct ic_2C02_registers ppu;
    struct ic_rp2a03 apu;
    struct controllers controllers;

    ic_6502_init(&cpu);
    ic_2c02_init(&ppu);
    ic_rp2a03_init(&apu);
    controllers_init(&controllers);
    cpu.mapper = mapper;
    ppu.mapper = mapper;

    mapper->cpu = &cpu;
    mapper->ppu = &ppu;
    mapper->apu = &apu;
    mapper->controllers = &controllers;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    SDL_Window *w = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 740, SDL_WINDOW_SHOWN);
    SDL_AudioSpec desired = {
        .freq = 44100,
        .format = AUDIO_F32,
        .channels = 1,
        .samples = 256,
        .callback = ic_rp2a03_sdl_audio_callback,
        .userdata = mapper,
    };

    SDL_AudioSpec obtained;
    SDL_AudioDeviceID audio = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    // TODO: Sanity checks
    if (audio == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    }

    mapper->sampling = obtained.freq;
    SDL_PauseAudioDevice(audio, 0);

    SDL_Surface *s = SDL_GetWindowSurface(w);

    TTF_Font *font = TTF_OpenFont("cour.ttf", 14);

    SDL_Surface *textSurface;
    SDL_Rect textLocation;
    SDL_Color fg = { 0xff, 0xff, 0xff, 0xff, };

    textLocation = (SDL_Rect){ .x = 2, .y = 256, .w = 0, .h = 0, };

    ic_2c02_reset(&ppu);
    ic_6502_reset(&cpu);

    uint32_t ticks[10] = {0};
    uint32_t counter = 0;
    uint32_t prev_ticks = 0;
    //int max_cycles = 1000000;
    while (1) {
        int render = mapper_clock(mapper);

        if (render) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                controllers_handle(&controllers, &event);
            }

            // Calculate FPS
            uint32_t total_ticks = 1; // Avoid division by 0
            for (int i = 0; i < 10; i++) {
                total_ticks += ticks[i];
            }
            uint32_t nticks = SDL_GetTicks();
            ticks[counter] = nticks - prev_ticks;
            prev_ticks = nticks;

            if (counter == 0) {
                counter = 9;
            } else {
                counter--;
            }

            // Render
            SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 0x00, 0x00, 0x00));

            for (int i = 0; i < 240; i++) {
                for (int j = 0; j < 256; j++) {
                    ((uint32_t *)s->pixels)[(i * s->w + j)] = palette_table[ppu.screen[i][j]];
                }
            }

            char fps_string[10];
            sprintf(fps_string, "%4.0f FPS", 1000.0f / (total_ticks / 10.0f));

            textSurface = TTF_RenderText_Solid(font, fps_string, fg);

            SDL_BlitSurface(textSurface, NULL, s, &textLocation);
            SDL_FreeSurface(textSurface);

            debug_draw(mapper, s);

            SDL_UpdateWindowSurface(w);
        }
    }

    return 0;
}

static void ic_rp2a03_sdl_audio_callback(void *userdata, uint8_t *stream_raw, int len)
{
    struct mapper *mapper = userdata;
    int flen = len / sizeof(float);
    float *stream = (float *)stream_raw;

    int length = mapper->buffer_tail - mapper->buffer_head;
    if (length < 0) length += 2048;
    if (length < flen) {
        printf("EMPTY\n");
        for (int i = 0; i < flen; i++) {
            stream[i] = 0.0f;
        }
        return;
    }

    for (int i = 0; i < flen; i++) {
        stream[i] = mapper->buffer[mapper->buffer_head];
        mapper->buffer_head = (mapper->buffer_head + 1) % 2048;
    }
}
