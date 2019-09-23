#include <stdint.h>
#include <string.h>
#include <math.h>
#include "SDL2/SDL.h"
#include "ic_rp2a03.h"

#define FREQ 48000.0

static uint8_t length_table[] = {
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static uint8_t duty_pattern[] = { 0x40, 0x60, 0x78, 0x9f };

void ic_rp2a03_init(struct ic_rp2a03 *apu)
{
    *apu = (struct ic_rp2a03) {
        .pulse = {
            (struct ic_rp2a03_pulse_channel) {
                .duty = 0,
                .counter_halt_envelope_loop = 0,
                .constant_volume = 0,
                .volume = 0,
                .sweep_shift = 0,
                .sweep_negate = 0,
                .sweep_period = 0,
                .sweep_enable = 0,
                .sweep_reload = 0,
                .timer = 0,
                .length = 0,

                .sweep_counter = 0,
                .duty_counter = 0,
                .time = 0,
                .value = 0,
            },

            (struct ic_rp2a03_pulse_channel) {
                .duty = 0,
                .counter_halt_envelope_loop = 0,
                .constant_volume = 0,
                .volume = 0,
                .sweep_shift = 0,
                .sweep_negate = 0,
                .sweep_period = 0,
                .sweep_enable = 0,
                .sweep_reload = 0,
                .timer = 0,
                .length = 0,

                .sweep_counter = 0,
                .duty_counter = 0,
                .time = 0,
                .value = 0,
            },
        },

        .buffer_head = 0,
        .buffer_tail = 0,
        .buffer_max = 0,
        .clocks = 0,

        .sampling = 0,
        .divider = 0,
        .frame_divider = 0,
    };
}

uint8_t ic_rp2a03_read(struct ic_rp2a03 *apu, uint16_t address)
{
    return 0;
}

void ic_rp2a03_write(struct ic_rp2a03 *apu, uint16_t address, uint8_t data)
{
    switch (address) {
        case 0x0000: case 0x0004:
            apu->pulse[address >> 2].duty = duty_pattern[data >> 6];
            apu->pulse[address >> 2].counter_halt_envelope_loop = (data & 0x20) >> 5;
            apu->pulse[address >> 2].constant_volume = (data & 0x10) >> 4;
            apu->pulse[address >> 2].volume = (data & 0x0f);
            break;
        case 0x0001: case 0x0005:
            apu->pulse[address >> 2].sweep_enable = (data & 0x80) >> 7;
            apu->pulse[address >> 2].sweep_period = (data & 0x70) >> 4;
            apu->pulse[address >> 2].sweep_negate = (data & 0x08) >> 3;
            apu->pulse[address >> 2].sweep_shift = (data & 0x07);
            apu->pulse[address >> 2].sweep_reload = 1;
            break;
        case 0x0002: case 0x0006:
            apu->pulse[address >> 2].timer &= 0xff00;
            apu->pulse[address >> 2].timer |= data;
            break;
        case 0x0003: case 0x0007:
            apu->pulse[address >> 2].timer &= 0x00ff;
            apu->pulse[address >> 2].timer |= (data & 0x07) << 8;
            apu->pulse[address >> 2].time = 0;
            apu->pulse[address >> 2].length = length_table[data >> 3];
            apu->pulse[address >> 2].duty_counter = 0;
            apu->pulse[address >> 2].envelope_start = 1;
            break;
    }
}

void ic_rp2a03_clock(struct ic_rp2a03 *apu)
{
    if (apu->divider == 0) {
        apu->divider = 1;

        for (int i = 0; i < 2; i++) {
            if (apu->pulse[i].time == 0) {
                apu->pulse[i].time = apu->pulse[i].timer;
                apu->pulse[i].duty_counter = (apu->pulse[i].duty_counter + 1) % 8;

                if (apu->pulse[i].length > 0 && apu->pulse[i].timer >= 8) {
                    if (apu->pulse[i].duty & (1 << apu->pulse[i].duty_counter)) {
                        if (apu->pulse[i].constant_volume) {
                            apu->pulse[i].value = apu->pulse[i].volume;
                        } else {
                            apu->pulse[i].value = apu->pulse[i].envelope_decay_counter;
                        }
                    } else {
                        apu->pulse[i].value = 0;
                    }
                }
            } else {
                apu->pulse[i].time--;
            }
        }
    } else {
        apu->divider--;
    }

    if (apu->frame_divider == 0) {
        apu->frame_divider = 1789773/240;

        if (apu->frame_counter_mode == 0 && apu->frame_counter == 4) {
            apu->frame_counter_mode = 3;
        }

        if (apu->frame_counter % 2 == 1) {
            // clock triangles
            for (int i = 0; i < 2; i++) {
                if (apu->pulse[i].length > 0 && !apu->pulse[i].counter_halt_envelope_loop) {
                    apu->pulse[i].length--;
                }

                uint16_t change_amount = apu->pulse[i].timer >> apu->pulse[i].sweep_shift;
                if (apu->pulse[i].sweep_negate) {
                    // Channel 1 and 2 differ in how they negate the shift
                    change_amount = i == 0 ? ~change_amount : -change_amount;
                }

                if (apu->pulse[i].sweep_counter == 0 && apu->pulse[i].sweep_enable) {
                    apu->pulse[i].timer += change_amount;
                }

                if (apu->pulse[i].sweep_counter == 0 || apu->pulse[i].sweep_reload) {
                    apu->pulse[i].sweep_reload = 0;
                    apu->pulse[i].sweep_counter = apu->pulse[i].sweep_period;
                }

                if (apu->pulse[i].sweep_counter > 0) {
                    apu->pulse[i].sweep_counter--;
                }
            }
        }

        if (apu->frame_counter == 0) {
            apu->frame_counter = 4;
        } else {
            apu->frame_counter--;
        }

        for (int i = 0; i < 2; i++) {
            if (apu->pulse[i].envelope_start) {
                apu->pulse[i].envelope_start = 0;
                apu->pulse[i].envelope_decay_counter = 15;
                apu->pulse[i].envelope_divider = apu->pulse[i].volume;
            } else {
                if (apu->pulse[i].envelope_divider == 0) {
                    apu->pulse[i].envelope_divider = apu->pulse[i].volume;
                    if (apu->pulse[i].envelope_decay_counter == 0) {
                        if(apu->pulse[i].counter_halt_envelope_loop) {
                            apu->pulse[i].envelope_decay_counter = 15;
                        }
                    } else {
                        apu->pulse[i].envelope_decay_counter--;
                    }
                } else {
                    apu->pulse[i].envelope_divider--;
                }
            }
        }
    } else {
        apu->frame_divider--;
    }

    apu->clocks++;
    while (apu->clocks >= 0) {
        while ((apu->buffer_tail + 1) % 2048 == apu->buffer_head);

        apu->clocks -= 1789773.0f / apu->sampling;
        apu->buffer[apu->buffer_tail] = 95.88 / (8128.0 / (apu->pulse[0].value + apu->pulse[1].value) + 100);
        apu->buffer_tail = (apu->buffer_tail + 1) % 2048;
    }
}
