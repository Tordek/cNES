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

#include "mapper1.h"

struct mapper_1 {
    struct mapper base;
    uint8_t main_ram[0x0800];
    uint8_t vram[0x2000];
    uint8_t palette[0x20];
    uint8_t work_ram[0x8000];
    uint8_t control;
    uint32_t chr_bank_0;
    uint32_t chr_bank_1;
    uint32_t prg_bank;
    uint32_t last_prg_bank;
    uint8_t lsr;
    uint8_t const *prg_rom;
    uint8_t *chr_rom;
};

static uint32_t prg_rom_address(struct mapper_1 *mapper, uint16_t address)
{
    switch (mapper->control & 0x0c) {
        case 0x00: case 0x04:
            return (mapper->prg_bank & 0x38000) | (address & 0x7fff);
        case 0x08:
            if (address < 0xc000) {
                return address & 0x3fff;
            } else {
                return mapper->prg_bank | (address & 0x3fff);
            }
        case 0x0c:
            if (address < 0xc000) {
                return mapper->prg_bank | (address & 0x3fff);
            } else {
                return mapper->last_prg_bank | (address & 0x3fff);
            }
    }
}

static uint32_t chr_rom_address(struct mapper_1 *mapper, uint16_t address)
{
    if ((mapper->control & 0x10) == 0){
        return (mapper->chr_bank_0 & 0x1e000) | (address & 0x1fff);
    } else {
        if (address < 0x1000) {
            return mapper->chr_bank_0 | (address & 0x0fff);
        } else {
            return mapper->chr_bank_1 | (address & 0x0fff);
        }
    }
}

static uint16_t vram_address(struct mapper_1 *mapper, uint16_t address)
{
    switch (mapper->control & 0x03) {
        case 0:
            return address & 0x03ff;
        case 1:
            return 0x0800 | (address & 0x03ff);
        case 2:
            return address & 0x07ff;
        case 3:
            return address & 0x0bff;
    }
}

uint8_t mapper_1_cpu_bus_read(struct mapper *mapper_, uint16_t address)
{
    struct mapper_1 *mapper = (struct mapper_1 *)mapper_;

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
        return mapper->prg_rom[prg_rom_address(mapper, address)];
    }
}

void mapper_1_cpu_bus_write(struct mapper *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_1 *mapper = (struct mapper_1 *)mapper_;

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
        if (data & 0x80) {
            mapper->lsr = 0x20;
            mapper->control |= 0x0c;
        } else {
            mapper->lsr = (mapper->lsr >> 1) | ((data & 0x01) << 5);
            if (mapper->lsr & 0x01) {
                mapper->lsr >>= 1;
                if (address < 0xa000) {
                    mapper->control = mapper->lsr;
                } else if (address < 0xc000) {
                    mapper->chr_bank_0 = mapper->lsr << 12;
                } else if (address < 0xe000) {
                    mapper->chr_bank_1 = mapper->lsr << 12;
                } else {
                    mapper->prg_bank = mapper->lsr << 14;
                }
                mapper->lsr = 0x20;
            }
        }
    }
}

uint8_t mapper_1_ppu_bus_read(struct mapper *mapper_, uint16_t address)
{
    struct mapper_1 *mapper = (struct mapper_1 *)mapper_;

    if (address < 0x2000) {
        return mapper->chr_rom[chr_rom_address(mapper, address)];
    } else {
        return mapper->vram[vram_address(mapper, address)];
    }
}

void mapper_1_ppu_bus_write(struct mapper *mapper_, uint16_t address, uint8_t data)
{
    struct mapper_1 *mapper = (struct mapper_1 *)mapper_;

    if (address < 0x2000) {
        mapper->chr_rom[chr_rom_address(mapper, address)] = data;
    } else {
        mapper->vram[vram_address(mapper, address)] = data;
    }
}

struct mapper *mapper_1_builder(struct nes_rom *rom)
{
    struct mapper_1 *mapper;

    mapper = malloc(sizeof(struct mapper_1));
    *mapper = (struct mapper_1) {
        .base = (struct mapper) {
            .dma_write_time = 0,
            .ppu_bus_read = mapper_1_ppu_bus_read,
            .ppu_bus_write = mapper_1_ppu_bus_write,
            .cpu_bus_read = mapper_1_cpu_bus_read,
            .cpu_bus_write = mapper_1_cpu_bus_write,
        },
        .chr_rom = rom->chr_rom,
        .prg_rom = rom->prg_rom,
        .control = 0x0c,
        .chr_bank_0 = 0,
        .chr_bank_1 = 0x1,
        .last_prg_bank = (rom->prg_rom_size - 1) << 14,
    };

    return (struct mapper *)mapper;
}

