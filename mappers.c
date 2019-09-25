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

#include "mapper0.c"
// #include "mapper1.c"

int mapper_clock(struct mapper *mapper)
{
    int render = ic_2C02_clock(mapper->ppu);
    if (render && mapper->ppu->do_nmi) {
        ic_6502_nmi(mapper->cpu);
    }

    if (mapper->clock == 0) {
        mapper->clock = 2;

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
            ic_6502_clock(mapper->cpu);
        }
        ic_rp2a03_clock(mapper->apu);
    } else {
        mapper->clock--;
    }

    return render;
}

struct mapper *(*mappers[])(struct nes_rom *rom) = {
    mapper_0_builder,
    //mapper_1_builder,
};
