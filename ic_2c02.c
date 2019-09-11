#include <stdint.h>
#include "SDL2/SDL.h"
#include "mapper.h"
#include "ic_2c02.h"

int32_t const palette_table[128] = {
    0xff7c7c7c, 0xff0000fc, 0xff0000bc, 0xff4428bc, 0xff940084, 0xffa80020, 0xffa81000, 0xff881400, 0xff503000, 0xff007800, 0xff006800, 0xff005800, 0xff004058, 0xff000000, 0xff000000, 0xff000000,
    0xffbcbcbc, 0xff0078f8, 0xff0058f8, 0xff6844fc, 0xffd800cc, 0xffe40058, 0xfff83800, 0xffe45c10, 0xffac7c00, 0xff00b800, 0xff00a800, 0xff00a844, 0xff008888, 0xff000000, 0xff000000, 0xff000000,
    0xfff8f8f8, 0xff3cbcfc, 0xff6888fc, 0xff9878f8, 0xfff878f8, 0xfff85898, 0xfff87858, 0xfffca044, 0xfff8b800, 0xffb8f818, 0xff58d854, 0xff58f898, 0xff00e8d8, 0xff787878, 0xff000000, 0xff000000,
    0xfffcfcfc, 0xffa4e4fc, 0xffb8b8f8, 0xffd8b8f8, 0xfff8b8f8, 0xfff8a4c0, 0xfff0d0b0, 0xfffce0a8, 0xfff8d878, 0xffd8f878, 0xffb8f8b8, 0xffb8f8d8, 0xff00fcfc, 0xfff8d8f8, 0xff000000, 0xff000000,
};

void ic_2c02_reset(struct ic_2C02_registers *ppu)
{
    ppu->status = 0;
    ppu->vram_increment = 1;
    ppu->clock = 0;
}

void ic_2C02_clock(struct ic_2C02_registers *ppu, SDL_Surface *surface)
{
    int pixel = ppu->clock % 341;
    int scanline = ppu->clock / 341;

    if (scanline < 240 && 0 < pixel && pixel < 257) {
        // TODO: fine_x
        uint8_t color_id = (ppu->pattern_lo & 0x8000) >> 15;
        color_id |= (ppu->pattern_hi & 0x8000) >> 14;
        int8_t color_abs = color_id ? ppu->palette | color_id : 0;
        int32_t color = palette_table[ppu->mapper->ppu_bus_read(ppu->mapper, 0x3f00 | color_abs)];

        ((uint32_t*)surface->pixels)[(scanline * 800 + pixel)] = color;
    }

    if ((2 <= pixel && pixel <= 257) || (322 <= pixel && pixel <= 337)) {
        ppu->pattern_lo <<= 1;
        ppu->pattern_hi <<= 1;
    }

    if ((scanline < 240 || scanline == 261) && (pixel < 257 || pixel > 320)) {
        int chr_x = (pixel >> 3) + 2;
        int fetch_scanline = scanline;
        if (chr_x > 40) {
            chr_x -= 42;
            fetch_scanline += 1;
        }

        if (fetch_scanline == 261) {
            fetch_scanline = 0;
        }

        int chr_y = fetch_scanline >> 3;
        int attr_x = chr_x >> 2;
        int attr_y = chr_y >> 2;

        if (pixel == 0) {
            //
        } else if (pixel == 338 || pixel == 340) {
            //
        } else {
            switch (pixel & 0x07) {
                case 0x02:
                    // Load NT byte
                    // Shift 4 bits now to save 2 shifts later.
                    ppu->nametable_byte_next = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->nametable | (chr_y << 5) | chr_x) << 4;
                    break;
                case 0x04:
                    // Load AT byte
                    ppu->attribute_byte_next = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->nametable | 0x03c0 | (attr_y << 3) | attr_x);
                    break;
                case 0x06:
                    // Low BG tile
                    ppu->pattern_lo_next = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->background | ppu->nametable_byte_next | (fetch_scanline & 0x07));
                    break;
                case 0x00:
                    // High BG tile
                    ppu->pattern_hi_next = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->background | ppu->nametable_byte_next | (fetch_scanline & 0x07) | 0x08);

                    // Shift 2 bits now to save 8 later when drawing the pixel
                    if ((chr_x & 0x02) && (chr_y & 0x02)) {
                        ppu->palette = (ppu->attribute_byte & 0x30) >> 2;
                    } else if ((chr_x & 0x02) == 0 && (chr_y & 0x02)) {
                        ppu->palette = (ppu->attribute_byte & 0xC0) >> 4;
                    } else if ((chr_x & 0x02) && (chr_y & 0x02) == 0) {
                        ppu->palette = (ppu->attribute_byte & 0x03) << 2;
                    } else {
                        ppu->palette = (ppu->attribute_byte & 0x0C);
                    }

                    ppu->pattern_lo |= ppu->pattern_lo_next;
                    ppu->pattern_hi |= ppu->pattern_hi_next;
                    ppu->attribute_byte = ppu->attribute_byte_next;
                    break;
            }
        }
    }

    ppu->clock++;
    if (ppu->clock == 340 * 241) {
        ppu->status |= 0x80; // vblank
    } else if (ppu->clock == 340 * 262) {
        ppu->status &= 0x7f;
        ppu->clock = 0;
    }
}

uint8_t ic_2c02_read(void *device, uint16_t address)
{
    struct ic_2C02_registers *ppu = device;
    switch (address) {
        case 0x2002:
            uint8_t status = ppu->status;
            ppu->status &= 0x7F;
            ppu->addr_latch = 1;
            ppu->scroll_v = 0;
            return status;
        case 0x2007:
            uint8_t result = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->vram_address);
            ppu->vram_address += ppu->vram_increment;
            return result;
    }
    return 0;
}

void ic_2c02_write(void *device, uint16_t address, uint8_t value)
{
    struct ic_2C02_registers *ppu = device;
    switch (address) {
        case 0x2000:
            ppu->nametable = 0x2000 | (value & 0x03) << 9;
            if (value & 0x04) {
                ppu->vram_increment = 32;
            } else {
                ppu->vram_increment = 1;
            }
            if (value & 0x08) {
                //
            }
            if (value & 0x10) {
                ppu->background = 0x1000;
            } else {
                ppu->background = 0x0000;
            }

            if (value & 0x20) {
                //
            }

            if (value & 0x40) {
                //
            }

            if (value & 0x80) {
                ppu->do_nmi = 1;
            } else {
                ppu->do_nmi = 0;
            }
            break;
        case 0x2005:
            if (ppu->scroll_v) {
                ppu->scroll_y = value;
            } else {
                ppu->scroll_x = value;
            }
            ppu->scroll_v = !ppu->scroll_v;
            break;

        case 0x2006:
            if (ppu->addr_latch) {
                ppu->vram_address = value << 8;
            } else {
                ppu->vram_address |= value;
            }
            ppu->addr_latch = !ppu->addr_latch;
            break;
        case 0x2007:
            ppu->mapper->ppu_bus_write(ppu->mapper, ppu->vram_address, value);
            ppu->vram_address += ppu->vram_increment;
            break;
    }
}
