#include <stdint.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

#include "nes_header.h"
#include "mapper.h"
#include "mappers.h"

#include "controllers.h"
#include "ic_rp2a03.h"
#include "ic_6502.h"
#include "ic_2c02.h"
struct mapper_0 {
    struct mapper base;
    uint8_t main_ram[0x0800];
    uint8_t vram[0x1000];
    uint8_t palette[0x20];
    uint8_t work_ram[0x8000];
    uint16_t nametable_mirroring;
    uint16_t prg_rom_mirroring;
    uint8_t const *prg_rom;
    uint8_t const *chr_rom;
};

uint8_t mapper_0_cpu_bus_read(struct mapper *mapper_, uint16_t address)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        return mapper->main_ram[address & 0x07ff];
    } else if (address < 0x4000) {
        return ic_2c02_read(mapper->base.ppu, address & 0x0007);
    } else if (address == 0x4016 || address == 0x4017) {
        return controllers_read(mapper->base.controllers, address & 0x0001);
    } else if (address < 0x4020) {
        return ic_rp2a03_read(mapper->base.apu, address & 0x001f);
    } else if (address < 0x8000) {
        return mapper->work_ram[address - 0x4020];
    } else {
        return mapper->prg_rom[address & mapper->prg_rom_mirroring];
    }
}

void mapper_0_cpu_bus_write(struct mapper *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        mapper->main_ram[address & 0x07ff] = data;
    } else if (address < 0x4000) {
        ic_2c02_write(mapper->base.ppu, address & 0x0007, data);
    } else if (address == 0x4014) {
        mapper->base.dma_page = data << 8;
        mapper->base.dma_write_time = 513 + (mapper->base.cpu->cycles & 0x01);
    } else if (address == 0x4016 || address == 0x4017) {
        controllers_write(mapper->base.controllers, address & 0x0001, data);
    } else if (address < 0x4020) {
        ic_rp2a03_write(mapper->base.apu, address & 0x001f, data);
    } else if (address < 0x8000) {
        mapper->work_ram[address - 0x4020] = data;
    } else {
        // prg_rom is not writable; ignore.
    }
}

uint8_t mapper_0_ppu_bus_read(struct mapper *mapper_, uint16_t address)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        return mapper->chr_rom[address];
    } else {
        return mapper->vram[address & mapper->nametable_mirroring];
    }
}

void mapper_0_ppu_bus_write(struct mapper *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        // chr_rom is not writable; ignore.
    } else {
        mapper->vram[address & mapper->nametable_mirroring] = data;
    }
}

struct mapper *mapper_0_builder(struct nes_rom *rom)
{
    struct mapper_0 *mapper;

    mapper = malloc(sizeof(struct mapper_0));
    *mapper = (struct mapper_0) {
        .base = (struct mapper) {
            .dma_write_time = 0,
            .ppu_bus_read = mapper_0_ppu_bus_read,
            .ppu_bus_write = mapper_0_ppu_bus_write,
            .cpu_bus_read = mapper_0_cpu_bus_read,
            .cpu_bus_write = mapper_0_cpu_bus_write,
        },
        .chr_rom = rom->chr_rom,
        .prg_rom = rom->prg_rom,
        .prg_rom_mirroring = rom->prg_rom_size == 1 ? 0x3fff : 0x7fff,
        .nametable_mirroring = rom->mirroring ? 0x07FF : 0x0BFF,
    };

    return (struct mapper *)mapper;
}

int mapper_clock(struct mapper *mapper, SDL_Surface *s)
{
    while (mapper->apu->buffer_count == 1024);
    mapper->clock++;

    int render = ic_2C02_clock(mapper->ppu, s);
    if (render && mapper->ppu->do_nmi) {
        ic_6502_nmi(mapper->cpu);
    }

    if (mapper->clock % 3 == 0) {
        ic_rp2a03_clock(mapper->apu);

        if (mapper->dma_write_time > 512) {
            mapper->dma_write_time--;
        } else if (mapper->dma_write_time > 0) {
            mapper->dma_write_time--;
            if ((mapper->dma_write_time & 1) == 0) {
                uint16_t byte = 255 - (mapper->dma_write_time >> 1);
                uint8_t val = mapper->cpu_bus_read(mapper, mapper->dma_page | byte);
                mapper->cpu_bus_write(mapper, 0x2004, val);
            }
        } else {
            mapper->clock = 0;
            ic_6502_clock(mapper->cpu);
        }
    }

    return render;
}

struct mapper *(*mappers[])(struct nes_rom *rom) = {
    mapper_0_builder,
};
