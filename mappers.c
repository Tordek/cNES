#include <stdint.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

#include "nes_header.h"
#include "apu.h"
#include "mapper.h"
#include "mappers.h"
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
    } else if (address < 0x4020) {
        return apu_read(mapper->base.apu, address & 0x401f);
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
    } else if (address < 0x4020) {
        apu_write(mapper->base.apu, address & 0x401f, data);
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
    } else if (address < 0x3F00) {
        return mapper->vram[address & mapper->nametable_mirroring];
    } else if ((address & 0x0003) == 0x0000) {
        return mapper->palette[address & 0x000F];
    } else {
        return mapper->palette[address & 0x001F];
    }
}

void mapper_0_ppu_bus_write(struct mapper *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        // chr_rom is not writable; ignore.
    } else if (address < 0x3F00) {
        mapper->vram[address & mapper->nametable_mirroring] = data;
    } else if ((address & 0x0003) == 0x0000) {
        mapper->palette[address & 0x000F] = data;
    } else {
        mapper->palette[address & 0x001F] = data;
    }
}

struct mapper *mapper_0_builder(struct nes_rom *rom)
{
    struct mapper_0 *mapper;

    mapper = malloc(sizeof(struct mapper_0));
    mapper->base.dma_write_time = 0;
    mapper->base.ppu_bus_read = mapper_0_ppu_bus_read;
    mapper->base.ppu_bus_write = mapper_0_ppu_bus_write;
    mapper->base.cpu_bus_read = mapper_0_cpu_bus_read;
    mapper->base.cpu_bus_write = mapper_0_cpu_bus_write;
    mapper->chr_rom = rom->chr_rom;
    mapper->prg_rom = rom->prg_rom;
    if (rom->prg_rom_size == 1) {
        mapper->prg_rom_mirroring = 0x3fff;
    } else {
        mapper->prg_rom_mirroring = 0x7fff;
    }
    mapper->nametable_mirroring = rom->mirroring ? 0x07FF : 0x0BFF;

    return (struct mapper *)mapper;
}

int mapper_clock(struct mapper *mapper, SDL_Surface *s)
{
    mapper->clock++;

    ic_2C02_clock(mapper->ppu, s);

    int render = 0;
    if (mapper->ppu->scanline == 241 && mapper->ppu->pixel == 1) {
        if (mapper->ppu->do_nmi) {
            ic_6502_nmi(mapper->cpu);
            render = 1;
        }
    }

    if (mapper->clock % 3 == 0) {
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

struct mapper * (*mappers[])(struct nes_rom *rom) = {
    mapper_0_builder,
};
