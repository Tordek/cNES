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

#include "mapper0.h"
#include "mapper1.h"

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

        float sample = ic_rp2a03_clock(mapper->apu);

        mapper->clocks++;
        while (mapper->clocks >= 0) {
            mapper->clocks -= 1789773.0f / mapper->sampling;

            while ((mapper->buffer_tail + 1) % 2048 == mapper->buffer_head);
            mapper->buffer[mapper->buffer_tail] = sample;
            mapper->buffer_tail = (mapper->buffer_tail + 1) % 2048;
        }
    } else {
        mapper->clock--;
    }

    return render;
}

struct mapper *(*mappers[])(struct nes_rom *rom) = {
    mapper_0_builder,
    mapper_1_builder,
};
