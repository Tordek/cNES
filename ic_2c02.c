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

void inc_x(struct ic_2C02_registers *ppu);
void inc_y(struct ic_2C02_registers *ppu);

void ic_2c02_reset(struct ic_2C02_registers *ppu)
{
    ppu->palette = 0;
    ppu->palette_next = 0;
    ppu->pattern_hi = 0;
    ppu->pattern_lo = 0;
    ppu->status = 0;
    ppu->vram_increment = 1;
    ppu->clock = 0;
    ppu->pixel = 0;
    ppu->scanline = 0;
}

void ic_2C02_clock(struct ic_2C02_registers *ppu, SDL_Surface *surface)
{
    int scanline = ppu->scanline;
    int pixel = ppu->pixel;
    if (ppu->enable) {
        if (-1 < scanline && scanline < 240 && 0 < pixel && pixel < 257) {
            int8_t color_abs;

            if (ppu->enable) { // BG
                uint8_t color_id = ((ppu->pattern_lo << ppu->fine_x) & 0x8000) >> 15;
                color_id |= ((ppu->pattern_hi << ppu->fine_x) & 0x8000) >> 14;
                color_abs = color_id ? ppu->palette | color_id : 0;
            }

            if (ppu->enable) { // Sprites
                for (int i = 0; i < 8; i++) {
                    if (ppu->sprite_x[i] > 0) {
                        ppu->sprite_x[i]--;
                        continue;
                    }

                    uint8_t sprite_color = 0;
                    if (ppu->sprite_attributes[i] & 0x40) {
                        sprite_color |= ppu->sprite_pattern_lo[i] & 0x01;
                        sprite_color |= (ppu->sprite_pattern_hi[i] & 0x01) << 1;
                        ppu->sprite_pattern_hi[i] >>= 1;
                        ppu->sprite_pattern_lo[i] >>= 1;
                    } else {
                        sprite_color |= (ppu->sprite_pattern_lo[i] & 0x80) >> 7;
                        sprite_color |= (ppu->sprite_pattern_hi[i] & 0x80) >> 6;
                        ppu->sprite_pattern_hi[i] <<= 1;
                        ppu->sprite_pattern_lo[i] <<= 1;
                    }

                    if (ppu->sprite_id[i] == 0 && sprite_color && color_abs) {
                        ppu->status |= 0x40;
                    }

                    if (sprite_color && ((ppu->sprite_attributes[i] & 0x20) == 0 || color_abs == 0)) {
                        uint8_t palette = (ppu->sprite_attributes[i] << 2) & 0x0c;

                        color_abs = 0x10 | palette | sprite_color;
                    }
                }
            }

            int32_t color = palette_table[ppu->mapper->ppu_bus_read(ppu->mapper, 0x3f00 | color_abs)];
            ((uint32_t *)surface->pixels)[(scanline * surface->w + pixel - 1)] = color;
        }

        if ((2 <= pixel && pixel <= 257) || (322 <= pixel && pixel <= 337)) {
            ppu->pattern_lo <<= 1;
            ppu->pattern_hi <<= 1;
        }

        if (scanline < 240) {
            uint16_t tile_address = 0x2000 | (ppu->vram_address & 0x0FFF);
            uint16_t attribute_address = 0x23C0 | (ppu->vram_address & 0x0c00) | ((ppu->vram_address >> 4) & 0x0038) | ((ppu->vram_address >> 2) & 0x0007);

            if (257 <= pixel && pixel <= 320) {
                // idle
            } else if (pixel == 338 || pixel == 340) {
                // Garbage NT read.
                ppu->mapper->ppu_bus_read(ppu->mapper, tile_address);
            } else if (pixel > 0) {
                switch (pixel & 0x07) {
                    case 0x02:
                        // Load NT byte
                        // Shift 4 bits now to save 2 shifts later.
                        ppu->nametable_byte_next = ppu->mapper->ppu_bus_read(ppu->mapper, tile_address) << 4;
                        break;
                    case 0x04:
                        // Load AT byte
                        ppu->attribute_byte_next = ppu->mapper->ppu_bus_read(ppu->mapper, attribute_address);
                        break;
                    case 0x06:
                        // Low BG tile
                        ppu->pattern_lo_next = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->background | ppu->nametable_byte_next | (ppu->vram_address >> 12));
                        break;
                    case 0x00:
                        // High BG tile
                        ppu->pattern_hi_next = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->background | ppu->nametable_byte_next | (ppu->vram_address >> 12) | 0x08);

                        ppu->palette = ppu->palette_next;
                        ppu->pattern_lo |= ppu->pattern_lo_next;
                        ppu->pattern_hi |= ppu->pattern_hi_next;
                        ppu->attribute_byte = ppu->attribute_byte_next;

                        // Shift 2 bits now to save 8 later when drawing the pixel
                        switch (ppu->vram_address & 0x0042)
                        {
                            case 0x0000:
                                ppu->palette_next = (ppu->attribute_byte & 0x03) << 2;
                                break;
                            case 0x0002:
                                ppu->palette_next = (ppu->attribute_byte & 0x0C);
                                break;
                            case 0x0040:
                                ppu->palette_next = (ppu->attribute_byte & 0x30) >> 2;
                                break;
                            case 0x0042:
                                ppu->palette_next = (ppu->attribute_byte & 0xC0) >> 4;
                                break;
                        }

                        inc_x(ppu);
                        break;
                }
            }
        } else {
            //ppu->dma_addr = 0;
        }

        if (ppu->scanline < 240 && ppu->pixel == 256) {
            inc_y(ppu);
        }

        if (ppu->scanline < 240 && ppu->pixel == 257) {
            ppu->vram_address &= 0xFBE0;
            ppu->vram_address |= ppu->t & 0x041f;
        }

        if (ppu->scanline < 240 && (257 <= ppu->pixel && ppu->pixel <= 320)) {
            ppu->oam_addr = 0x00;
        }

        if (ppu->scanline == -1 && 280 <= ppu->pixel && ppu->pixel <= 304) {
            ppu->vram_address &= 0x041f;
            ppu->vram_address |= ppu->t & 0xfbe0;
        }
    }

    // Sprite evaluation
    if (pixel < 65) {
        ppu->secondary_oam.raw[pixel & 0x1f] = 0xFF;
        ppu->secondary_oam_index = 0;
        ppu->n = 0;
        ppu->oam_state = 1;
        ppu->waste = 1;
    } else if (pixel < 256) {
        switch (ppu->oam_state) {
            case 1:
            {
                // 1
                struct oam_value cur_oam = ppu->primary_oam.data[ppu->n];
                uint8_t sprite_y = cur_oam.y;
                if (ppu->secondary_oam_index < 8) {
                    ppu->secondary_oam.data[ppu->secondary_oam_index].y = sprite_y;
                    // 1a
                    if (sprite_y <= scanline && scanline < (sprite_y + 8)) {
                        ppu->secondary_oam.data[ppu->secondary_oam_index] = ppu->primary_oam.data[ppu->n];
                        ppu->sprite_id[ppu->secondary_oam_index] = ppu->n;
                        ppu->secondary_oam_index++;
                    }
                }
                ppu->n++;
                ppu->oam_state = 2;
            }
                break;

            case 2:
            {
                if (ppu->n == 64) {
                    ppu->n = 0;
                    ppu->oam_state = 4;
                } else if (ppu->secondary_oam_index < 8) {
                    ppu->oam_state = 1;
                } else if (ppu->secondary_oam_index == 8) {
                    ppu->m = 0;
                    ppu->oam_state = 3;
                }
            }
                break;

            case 3:
            {
                uint8_t y = ppu->primary_oam.raw[ppu->n * 4 + ppu->m];
                if (ppu->waste || (y <= scanline && scanline < (y + 8))) {
                    ppu->waste = 1;
                    ppu->status |= 0x20;
                    ppu->m++;
                    if (ppu->m == 4) {
                        ppu->m = 0;
                        ppu->n++;
                        ppu->waste = 0;
                    }
                } else {
                    ppu->m = (ppu->m + 1) & 0x07;
                    ppu->n++;
                }

                if (ppu->n == 64) {
                    ppu->n = 0;
                    ppu->oam_state = 4;
                }
            }
                break;

            case 4:
                    ppu->n = (ppu->n + 1) & 0x3f;
        }
    } else if (pixel < 265) { // For absolute cycle-accuracy, do this in different cycles.
        struct oam_value sprite = ppu->secondary_oam.data[pixel & 0x07];
        ppu->sprite_pattern_lo[pixel & 0x07] = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->sprite_pattern_table | (sprite.tile_index << 4) + (scanline - sprite.y) );
        ppu->sprite_pattern_hi[pixel & 0x07] = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->sprite_pattern_table | (sprite.tile_index << 4) + (scanline - sprite.y + 8));
        ppu->sprite_attributes[pixel & 0x07] = sprite.attributes;
        ppu->sprite_x[pixel & 0x07] = sprite.x;
    } else if (pixel == 340) {
    }

    ppu->clock++;
    ppu->pixel++;

    if (ppu->pixel > 340) {
        ppu->pixel = 0;
        ppu->scanline += 1;

        if (ppu->scanline > 260) {
            ppu->scanline = -1;
        }
    }

    if (ppu->scanline == 241 && ppu->pixel == 1) {
        ppu->status |= 0x80; // Enter VBlank
    }

    if (ppu->scanline == -1 && ppu->pixel == 1) {
        ppu->status &= 0x7f; // Exit VBlank
        ppu->status &= 0xbf;
        ppu->clock = 0;
    }
}

uint8_t ic_2c02_read(struct ic_2C02_registers *device, uint16_t address)
{
    struct ic_2C02_registers *ppu = device;
    switch (address) {
        case 0x0002:
            uint8_t status = ppu->status;
            ppu->status &= 0x7F;
            ppu->w = 0;
            return status;
        case 0x0007:
            uint8_t result = ppu->ppudata_read;
            ppu->ppudata_read = ppu->mapper->ppu_bus_read(ppu->mapper, ppu->vram_address & 0x3FFF);
            if (ppu->enable && ppu->scanline < 240) {
                inc_x(ppu);
                inc_y(ppu);
            } else {
                ppu->vram_address += ppu->vram_increment;
            }
            return result;
    }
    return 0;
}

void ic_2c02_write(struct ic_2C02_registers *device, uint16_t address, uint8_t value)
{
    struct ic_2C02_registers *ppu = device;
    switch (address) {
        case 0x0000:
            ppu->t &= 0xF3FF;
            ppu->t |= (value & 0x03) << 10;

            if (value & 0x04) {
                ppu->vram_increment = 32;
            } else {
                ppu->vram_increment = 1;
            }

            ppu->sprite_pattern_table = (value & 0x08) << 9;

            ppu->background = (value & 0x10) << 8;

            ppu->tall_sprites = (value & 0x20);

            if (value & 0x40) {
                //
            }

            ppu->do_nmi = value & 0x80;

            break;
        case 0x0001:
            ppu->enable = value & 0x08;
            break;
        case 0x0003:
            ppu->oam_addr = value;
            break;
        case 0x0004:
            ppu->primary_oam.raw[ppu->oam_addr] = value;
            ppu->oam_addr = ppu->oam_addr + 1;
            break;
        case 0x0005:
            if (ppu->w == 0) {
                ppu->t &= 0xFFE0;
                ppu->t |= value >> 3;
                ppu->fine_x = value & 0x07;
                ppu->w = 1;
            } else {
                ppu->t &= 0x0C1F;
                ppu->t |= (value & 0xF8) << 2;
                ppu->t |= (value & 0x07) << 12;
                ppu->w = 0;
            }
            break;
        case 0x0006:
            if (ppu->w == 0) {
                ppu->t &= 0x00FF;
                ppu->t |= (value & 0x3F) << 8;
                ppu->w = 1;
            } else {
                ppu->t &= 0xFF00;
                ppu->t |= value;
                ppu->vram_address = ppu->t;
                ppu->w = 0;
            }
            break;
        case 0x0007:
            ppu->mapper->ppu_bus_write(ppu->mapper, ppu->vram_address & 0x3FFF, value);
            if (ppu->enable && ppu->scanline < 240) {
                inc_x(ppu);
                inc_y(ppu);
            } else {
                ppu->vram_address += ppu->vram_increment;
            }
            break;
    }
}

void inc_x(struct ic_2C02_registers *ppu)
{
    if ((ppu->vram_address & 0x001F) == 0x001F) {
        ppu->vram_address &= 0xFFE0;
        ppu->vram_address ^= 0x0400;
    } else {
        ppu->vram_address++;
    }
}

void inc_y(struct ic_2C02_registers *ppu)
{
    if ((ppu->vram_address & 0x7000) == 0x7000) {
        ppu->vram_address &= 0x0fff;
        uint16_t y = (ppu->vram_address & 0x03e0) >> 5;
        if (y == 29) {
            y = 0;
            ppu->vram_address ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y++;
        }
        ppu->vram_address = (ppu->vram_address & 0xFC1F) | (y << 5);
    } else {
        ppu->vram_address += 0x1000;
    }
}
