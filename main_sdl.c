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

int mapper_clock(struct mapper *mapper, SDL_Surface *s);

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
        .samples = 1024,
        .callback = ic_rp2a03_sdl_audio_callback,
        .userdata = &apu,
    };

    SDL_AudioSpec obtained;
    SDL_AudioDeviceID audio = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    // TODO: Sanity checks
    if (audio == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    }

    apu.sampling = obtained.freq;
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
    uint32_t counter = 10;
    uint32_t prev_ticks = 0;
    //int max_cycles = 1000000;
    while (1) {
        int render = mapper_clock(mapper, s);

        if (render) {
            SDL_UpdateWindowSurface(w);
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                controllers_handle(&controllers, &event);
            }
            SDL_FillRect( s, NULL, SDL_MapRGB( s->format, 0x00, 0x00, 0x00 ) );

            debug_draw(mapper, s);

            uint32_t total_ticks = 0;
            for (int i = 0; i < 10; i++) {
                total_ticks += ticks[i];
            }
            uint32_t nticks = SDL_GetTicks();
            ticks[counter] = nticks - prev_ticks;
            prev_ticks = nticks;
            char fps_string[10];
            sprintf(fps_string, "%4.0f FPS", 1000.0f / (total_ticks / 10.0f));

            textSurface = TTF_RenderText_Solid(font, fps_string, fg);

            SDL_BlitSurface(textSurface, NULL, s, &textLocation);
            SDL_FreeSurface(textSurface);

            if (counter-- == 0) {
                counter = 9;
            }
        }
    }

    return 0;
}
