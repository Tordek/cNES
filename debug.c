
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "nes_header.h"
#include "mapper.h"
#include "ic_6502.h"
#include "ic_2c02.h"
#include "ic_rp2a03.h"

#include "debug.h"

#define AUDIO_SAMPLES 100

static uint8_t pulse_0[AUDIO_SAMPLES] = {0};
static uint8_t pulse_1[AUDIO_SAMPLES] = {0};
static uint8_t triangle[AUDIO_SAMPLES] = {0};
static uint8_t noise[AUDIO_SAMPLES] = {0};

static int32_t const palette_table[128] = {
    0xff7c7c7c, 0xff0000fc, 0xff0000bc, 0xff4428bc, 0xff940084, 0xffa80020, 0xffa81000, 0xff881400, 0xff503000, 0xff007800, 0xff006800, 0xff005800, 0xff004058, 0xff000000, 0xff000000, 0xff000000,
    0xffbcbcbc, 0xff0078f8, 0xff0058f8, 0xff6844fc, 0xffd800cc, 0xffe40058, 0xfff83800, 0xffe45c10, 0xffac7c00, 0xff00b800, 0xff00a800, 0xff00a844, 0xff008888, 0xff000000, 0xff000000, 0xff000000,
    0xfff8f8f8, 0xff3cbcfc, 0xff6888fc, 0xff9878f8, 0xfff878f8, 0xfff85898, 0xfff87858, 0xfffca044, 0xfff8b800, 0xffb8f818, 0xff58d854, 0xff58f898, 0xff00e8d8, 0xff787878, 0xff000000, 0xff000000,
    0xfffcfcfc, 0xffa4e4fc, 0xffb8b8f8, 0xffd8b8f8, 0xfff8b8f8, 0xfff8a4c0, 0xfff0d0b0, 0xfffce0a8, 0xfff8d878, 0xffd8f878, 0xffb8f8b8, 0xffb8f8d8, 0xff00fcfc, 0xfff8d8f8, 0xff000000, 0xff000000,
};

void debug_draw(struct mapper *mapper, SDL_Surface *surface)
{
    TTF_Font *font = TTF_OpenFont("cour.ttf", 10);

    SDL_Surface *textSurface;
    SDL_Rect textLocation;
    SDL_Color fg = { 0xff, 0xff, 0xff, 0xff, };

    textLocation = (SDL_Rect){ .x = 256 + 16, .y = 2, .w = 0, .h = 0, };
    textSurface = TTF_RenderText_Solid(
        font,
        "BG",
        fg
    );

    SDL_BlitSurface(textSurface, NULL, surface, &textLocation);
    SDL_FreeSurface(textSurface);

    textLocation = (SDL_Rect){ .x = 256 + 16 + 64, .y = 2, .w = 0, .h = 0, };
    textSurface = TTF_RenderText_Solid(
        font,
        "Sprite palette",
        fg
    );

    SDL_BlitSurface(textSurface, NULL, surface, &textLocation);
    SDL_FreeSurface(textSurface);
    // Palettes
    for (int i = 0; i < 8; i++) {       // Palette ID
        for (int j = 0; j < 4; j++) {   // Color
            uint8_t color = mapper->ppu->palette[(i << 2) | j];
            SDL_Rect p = {
                .x = i * 16 + 256 + 16,
                .y = j * 16 + 16,
                .w = 12,
                .h = 12,
            };
            SDL_FillRect(surface, &p, palette_table[color]);
        }
    }

    textLocation = (SDL_Rect){ .x = 256 + 16, .y = 82, .w = 0, .h = 0, };
    textSurface = TTF_RenderText_Solid(
        font,
        "Pattern tables",
        fg
    );

    SDL_BlitSurface(textSurface, NULL, surface, &textLocation);
    SDL_FreeSurface(textSurface);

    // Pattern tables
    for (int n = 0; n < 2; n++) {                 // Pattern table
        for (int j = 0; j < 16; j++) {            // Coarse Y
            for (int i = 0; i < 16; i++) {        // Coarse X
                for (int y = 0; y < 8; y++) {     // Fine Y
                    uint8_t pattern_hi = mapper->ppu_bus_read(mapper, (n << 12) | (j << 8) | (i << 4) | y + 8);
                    uint8_t pattern_lo = mapper->ppu_bus_read(mapper, (n << 12) | (j << 8) | (i << 4) | y);

                    for (int x = 0; x < 8; x++) { // Fine X
                        int p_x = 272 + (n * 128) + (i * 8) + x;
                        int p_y = 96 + (j * 8) + y;

                        int color_id = (pattern_lo & 0x80) >> 7;
                        color_id |= (pattern_hi & 0x80) >> 6;

                        // TODO: Can change palette.
                        ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = palette_table[mapper->ppu->palette[color_id]];

                        pattern_hi <<= 1;
                        pattern_lo <<= 1;
                    }
                }
            }
        }
    }

    textLocation = (SDL_Rect){ .x = 256 + 16, .y = 224, .w = 0, .h = 0, };
    textSurface = TTF_RenderText_Solid(
        font,
        "Nametables",
        fg
    );

    SDL_BlitSurface(textSurface, NULL, surface, &textLocation);
    SDL_FreeSurface(textSurface);

    // Nametables
    for (int n = 0; n < 2; n++) {                 // Nametable Y
        for (int m = 0; m < 2; m++) {             // Nametable X
            for (int j = 0; j < 30; j++) {        // Coarse Y
                for (int i = 0; i < 32; i++) {    // Coarse X
                    uint16_t nametable = (n << 11) | (m << 10);
                    uint16_t tileset = mapper->ppu->background;

                    uint16_t tile_location = nametable | (j << 5) | i;
                    uint16_t attribute_location = nametable | 0x03c0 | ((j & 0xFC) << 1) | (i >> 2);

                    uint8_t chr_index = mapper->ppu_bus_read(mapper, 0x2000 | tile_location);
                    uint8_t attribute = mapper->ppu_bus_read(mapper, 0x2000 | attribute_location);

                    uint16_t palette = 0x00;

                    switch (tile_location & 0x0042) {
                        case 0x0000:
                            palette = (attribute & 0x03) << 2;
                            break;
                        case 0x0002:
                            palette = (attribute & 0x0C);
                            break;
                        case 0x0040:
                            palette = (attribute & 0x30) >> 2;
                            break;
                        case 0x0042:
                            palette = (attribute & 0xC0) >> 4;
                            break;
                    }

                    for (int y = 0; y < 8; y++) {      // Fine Y
                        uint8_t pattern_hi = mapper->ppu_bus_read(mapper, tileset | (chr_index << 4) | y | 8);
                        uint8_t pattern_lo = mapper->ppu_bus_read(mapper, tileset | (chr_index << 4) | y);

                        for (int x = 0; x < 8; x++) {  // Fine X
                            int p_x = 256 + 16 + (256 * m) + (8 * i) + x;
                            int p_y = 240 + (240 * n) + (8 * j) + y;

                            int color_id = (pattern_lo & 0x80) >> 7;
                            color_id |= (pattern_hi & 0x80) >> 6;

                            int color_abs = color_id ? palette | color_id : 0;

                            ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = palette_table[mapper->ppu->palette[color_abs]];

                            pattern_hi <<= 1;
                            pattern_lo <<= 1;
                        }
                    }
                }
            }
        }
    }

    textLocation = (SDL_Rect){ .x = 448, .y = 2, .w = 0, .h = 0, };
    textSurface = TTF_RenderText_Solid(
        font,
        mapper->ppu->tall_sprites ? "OAM (8x16)" : "OAM (8x8)",
        fg
    );

    SDL_BlitSurface(textSurface, NULL, surface, &textLocation);
    SDL_FreeSurface(textSurface);

    // OAM
    for (int n = 0; n < 64; n++) {
        struct oam_value sprite = mapper->ppu->primary_oam.data[n];
        for (int y = 0; y < 8; y++) {
            uint8_t pattern_hi = mapper->ppu_bus_read(mapper, mapper->ppu->sprite_pattern_table | (sprite.tile_index << 4) | y | 8);
            uint8_t pattern_lo = mapper->ppu_bus_read(mapper, mapper->ppu->sprite_pattern_table | (sprite.tile_index << 4) | y);

            for (int x = 0; x < 8; x++) {
                int p_x = ((n % 8) * 8) + x + 448;
                int p_y = ((n / 8) * 8) + y + 16;

                int color_id = (pattern_lo & 0x80) >> 7;
                color_id |= (pattern_hi & 0x80) >> 6;
                color_id |= (sprite.attributes & 0x03) << 2;
                color_id |= 0x10;

                ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = palette_table[mapper->ppu->palette[color_id]];

                pattern_hi <<= 1;
                pattern_lo <<= 1;
            }
        }
    }

    textLocation = (SDL_Rect){ .x = 2, .y = 300, .w = 0, .h = 0, };
    textSurface = TTF_RenderText_Solid(
        font,
        "Audio channels",
        fg
    );

    SDL_BlitSurface(textSurface, NULL, surface, &textLocation);
    SDL_FreeSurface(textSurface);

    for (int i = 0; i < AUDIO_SAMPLES - 1; i++) {
        pulse_0[i] = pulse_0[i+1];
        pulse_1[i] = pulse_1[i+1];
        triangle[i] = triangle[i+1];
        noise[i] = noise[i+1];
    }

    pulse_0[AUDIO_SAMPLES - 1] = mapper->apu->pulse[0].length == 0 ? 0 :
                                 mapper->apu->pulse[0].constant_volume ? mapper->apu->pulse[0].volume : mapper->apu->pulse[0].envelope_decay_counter;
    pulse_1[AUDIO_SAMPLES - 1] = mapper->apu->pulse[1].length == 0 ? 0 :
                                 mapper->apu->pulse[1].constant_volume ? mapper->apu->pulse[1].volume : mapper->apu->pulse[1].envelope_decay_counter;
    triangle[AUDIO_SAMPLES - 1] = mapper->apu->triangle_length > 0 && mapper->apu->triangle_linear_counter > 0 ? 15 : 0;
    noise[AUDIO_SAMPLES - 1] = mapper->apu->noise_length == 0 ? 0 :
                               mapper->apu->noise_constant_volume ? mapper->apu->noise_volume : mapper->apu->noise_envelope_decay_counter;

    for (int i = 0; i < AUDIO_SAMPLES; i++) {
        int p_x;
        int p_y;

        p_x = i + 4;
        p_y = 350 - pulse_0[i] * 2;
        ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = 0xFFFFFFFF;

        p_x = i + 4;
        p_y = 420 - pulse_1[i] * 2;
        ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = 0xFFFFFFFF;

        p_x = i + 4;
        p_y = 490 - triangle[i] * 2;
        ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = 0xFFFFFFFF;

        p_x = i + 4;
        p_y = 560 - noise[i] * 2;
        ((uint32_t *)surface->pixels)[(p_y * surface->w + p_x)] = 0xFFFFFFFF;

    }

    // OAM

    TTF_CloseFont(font);
}
