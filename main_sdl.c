#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>

#include "SDL2/SDL.h"

#include "nes_header.h"
#include "apu.h"
#include "mapper.h"
#include "ic_6502.h"
#include "ic_2c02.h"

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

    struct ic_6502_registers cpu;
    struct ic_2C02_registers ppu;
    struct apu apu;

    struct mapper *mapper = (mappers[rom.mapper_id])(&rom);
    mapper->cpu = &cpu;
    mapper->ppu = &ppu;
    mapper->apu = &apu;
    cpu.mapper = mapper;
    ppu.mapper = mapper;

    ic_2c02_reset(&ppu);
    ic_6502_reset(&cpu);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *w = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);

    SDL_Surface *s = SDL_GetWindowSurface(w);

    uint32_t total_cycles = 0;
    uint32_t ticks = 0;
    uint32_t counter = 10;
    while (1) {
        do {
            int render = 0;
            ic_2C02_clock(&ppu, s);
            if (ppu.clock == 0) {
                render = 1;
                if (ppu.do_nmi) ic_6502_nmi(&cpu);
            }
            ic_2C02_clock(&ppu, s);
            if (ppu.clock == 0) {
                render = 1;
                if (ppu.do_nmi) ic_6502_nmi(&cpu);
            }
            ic_2C02_clock(&ppu, s);
            if (ppu.clock == 0) {
                render = 1;
                if (ppu.do_nmi) ic_6502_nmi(&cpu);
            }
            ic_6502_clock(&cpu);
            total_cycles += 1;

            if (render) {
                render = 0;

                SDL_UpdateWindowSurface(w);
                SDL_Event event;
                while (SDL_PollEvent(&event));
                SDL_FillRect( s, NULL, SDL_MapRGB( s->format, 0xFF, 0xFF, 0xFF ) );
                if (counter-- == 0) {
                    uint32_t newticks = SDL_GetTicks();
                    float fps = 10000.0f / (newticks - ticks);
                    ticks = newticks;
                    printf("Reset FPS %f\n", fps);
                    counter = 10;
                }
                //SDL_Delay(13);
            }
        } while(cpu.cycles > 0);
    }

    return 0;
}
