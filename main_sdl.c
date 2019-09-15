#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "nes_header.h"
#include "apu.h"
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
    struct apu apu;
    mapper->cpu = &cpu;
    mapper->ppu = &ppu;
    mapper->apu = &apu;
    cpu.mapper = mapper;
    ppu.mapper = mapper;

    ic_2c02_reset(&ppu);
    ic_6502_reset(&cpu);

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *w = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 740, SDL_WINDOW_SHOWN);

    SDL_Surface *s = SDL_GetWindowSurface(w);

    TTF_Font *font = TTF_OpenFont("cour.ttf", 14);

    SDL_Surface *textSurface;
    SDL_Rect textLocation;
    SDL_Color fg = { 0xff, 0xff, 0xff, 0xff, };

    textLocation = (SDL_Rect){ .x = 2, .y = 256, .w = 0, .h = 0, };

    uint32_t ticks[10] = {0};
    uint32_t counter = 10;
    uint32_t prev_ticks = 0;
    //int max_cycles = 1000000;
    while (1) {
        do {
            int render = mapper_clock(mapper, s);

            if (render) {
                SDL_UpdateWindowSurface(w);
                SDL_Event event;
                while (SDL_PollEvent(&event));
                SDL_FillRect( s, NULL, SDL_MapRGB( s->format, 0x00, 0x00, 0x00 ) );

                debug_draw(mapper, s);

                uint32_t total_ticks = 0;
                for (int i = 0; i < 10; i++) {
                    total_ticks += ticks[i];
                }
                ticks[counter] = SDL_GetTicks() - prev_ticks;
                prev_ticks = SDL_GetTicks();
                char fps_string[10];
                sprintf(fps_string, "%4.0f FPS", 10000.0f / total_ticks);

                textSurface = TTF_RenderText_Solid( font, fps_string, fg);

                SDL_BlitSurface(textSurface, NULL, s, &textLocation);
                SDL_FreeSurface(textSurface);

                if (counter-- == 0) {
                    counter = 9;
                }
                //SDL_Delay(13);
            }
        } while(cpu.cycles > 0);
    }

    return 0;
}
