#include <stdint.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

#include "nes_header.h"
#include "apu.h"
#include "mapper.h"
#include "ic_6502.h"
#include "ic_2c02.h"

struct mapper_0 {
    struct mapper base;
    uint8_t main_ram[0x0800];
    uint8_t vram[0x2000];
    uint8_t work_ram[0x8000];
    uint16_t nametable_mirroring;
    uint8_t *prg_rom;
    uint8_t *chr_rom;
};

uint8_t mapper_0_cpu_bus_read(void *mapper_, uint16_t address)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        return mapper->main_ram[address & 0x07ff];
    } else if (address < 0x4000) {
        return ic_2c02_read(mapper->base.ppu, address & 0x2007);
    } else if (address < 0x4020) {
        return apu_read(mapper->base.apu, address & 0x401f);
    } else if (address < 0x8000) {
        return mapper->work_ram[address - 0x4020];
    } else {
        return mapper->prg_rom[address & 0x3FFF];
    }
}

void mapper_0_cpu_bus_write(void *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        mapper->main_ram[address & 0x07ff] = data;
    } else if (address < 0x4000) {
        ic_2c02_write(mapper->base.ppu, address & 0x2007, data);
    } else if (address < 0x4020) {
        apu_write(mapper->base.apu, address & 0x401f, data);
    } else if (address < 0x8000) {
        mapper->work_ram[address - 0x4020] = data;
    } else {
        // prg_rom is not writable; ignore.
    }
}

uint8_t mapper_0_ppu_bus_read(void *mapper_, uint16_t address)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        return mapper->chr_rom[address];
    } else if (address < 0x3000) {
        return mapper->vram[address & mapper->nametable_mirroring];
    } else {
        return mapper->vram[address & 0x1FFF];
    }
}

void mapper_0_ppu_bus_write(void *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_0 *mapper = (struct mapper_0 *)mapper_;

    if (address < 0x2000) {
        // chr_rom is not writable; ignore.
    } else if (address < 0x3000) {
        mapper->vram[address & mapper->nametable_mirroring] = data;
    } else {
        mapper->vram[address & 0x1FFF] = data;
    }
}

struct mapper *mapper_0_builder(struct nes_rom *rom)
{
    struct mapper_0 *mapper;

    mapper = malloc(sizeof(struct mapper_0));
    mapper->base.ppu_bus_read = mapper_0_ppu_bus_read;
    mapper->base.ppu_bus_write = mapper_0_ppu_bus_write;
    mapper->base.cpu_bus_read = mapper_0_cpu_bus_read;
    mapper->base.cpu_bus_write = mapper_0_cpu_bus_write;
    mapper->chr_rom = rom->chr_rom;
    mapper->prg_rom = rom->prg_rom;
    mapper->nametable_mirroring = rom->mirroring ? 0x17FF : 0x0FFF;

    return (struct mapper *)mapper;
}

struct mapper * (*mappers[])(struct nes_rom *rom) = {
    mapper_0_builder,
};
