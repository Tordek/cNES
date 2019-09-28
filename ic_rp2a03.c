#include <stdint.h>
#include <string.h>
#include <math.h>
#include "SDL2/SDL.h"
#include "ic_rp2a03.h"

#define FREQ 48000.0

static uint8_t const length_table[] = {
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static uint8_t const triangle_sequencer[] = {
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
};

static uint16_t const noise_period[] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
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
        .triangle_sequence = 0,
        .noise_lfsr = 1,

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
    switch (address) {
        case 0x0015:
        {
            uint8_t result = 0;
            if (apu->pulse[0].length > 0) {
                result |= 0x01;
            }

            if (apu->pulse[1].length > 0) {
                result |= 0x02;
            }

            if (apu->triangle_length > 0) {
                result |= 0x04;
            }

            if (apu->noise_length > 0) {
                result |= 0x08;
            }
            return result;
        }
            break;
    }
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
            apu->pulse[address >> 2].length = length_table[data >> 3];

            apu->pulse[address >> 2].duty_counter = 0;
            apu->pulse[address >> 2].envelope_start = 1;
            apu->pulse[address >> 2].sweep_mute = 0;
            break;
        case 0x0008:
            apu->triangle_counter_halt = data >> 7;
            apu->triangle_counter_load = data & 0x7f;
            break;
        case 0x0009:
            // Unused
            break;
        case 0x000a:
            apu->triangle_timer &= 0xff00;
            apu->triangle_timer |= data;
            break;
        case 0x000b:
            apu->triangle_timer &= 0x00ff;
            apu->triangle_timer |= (data & 0x07) << 8;
            apu->triangle_time = 0;
            apu->triangle_length = length_table[data >> 3];
            apu->triangle_counter_reload = 1;
            break;
        case 0x000c:
            apu->noise_counter_halt_envelope_loop = (data & 0x20) >> 5;
            apu->noise_constant_volume = (data & 0x10) >> 4;
            apu->noise_volume = data & 0x0f;
            break;
        case 0x000d:
            // Unused
            break;
        case 0x000e:
            apu->noise_mode = (data & 0x80) >> 7;
            apu->noise_timer = noise_period[data & 0x0f];
            break;
        case 0x000f:
            apu->noise_counter_load = data >> 3;
            apu->noise_length = apu->noise_counter_load;
            apu->noise_envelope_start = 1;
            break;

        case 0x0010:
            apu->dmc_irq_enable = (data & 0x80) >> 7;
            apu->dmc_loop = (data & 0x40) >> 6;
            apu->dmc_frequency = data & 0x0f;
            break;
        case 0x0011:
            apu->dmc_load_counter = data & 0x7f;
            break;
        case 0x0012:
            apu->dmc_sample_address = data;
            break;
        case 0x0013:
            apu->dmc_sample_length = data;
            break;
        case 0x0015:
            if ((data & 0x08) == 0) {
                apu->noise_value = 0;
                apu->noise_length = 0;
            }

            if ((data & 0x04) == 0) {
                apu->triangle_value = 0;
                apu->triangle_length = 0;
            }

            if ((data & 0x02) == 0) {
                apu->pulse[1].value = 0;
                apu->pulse[1].length = 0;
            }

            if ((data & 0x01) == 0) {
                apu->pulse[0].value = 0;
                apu->pulse[0].length = 0;
            }

            break;
        case 0x0017:
            apu->frame_counter_mode = data >> 7;
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

                if ((apu->pulse[i].duty & (1 << apu->pulse[i].duty_counter)) == 0
                        || apu->pulse[i].sweep_mute
                        || apu->pulse[i].length == 0
                        || apu->pulse[i].timer < 8) {
                    apu->pulse[i].value = 0;
                } else {
                    apu->pulse[i].value = apu->pulse[i].constant_volume ? apu->pulse[i].volume : apu->pulse[i].envelope_decay_counter;
                }
            } else {
                apu->pulse[i].time--;
            }
        }

        uint16_t feedback_bit = apu->noise_lfsr >> (apu->noise_mode ? 6 : 1) & 0x0001;
        feedback_bit ^= apu->noise_lfsr & 0x0001;
        apu->noise_lfsr = feedback_bit << 14 | apu->noise_lfsr >> 1;
        if (apu->noise_length == 0 || (apu->noise_lfsr & 0x0001)) {
            apu->noise_value = 0;
        } else {
            apu->noise_value = apu->noise_constant_volume ? apu->noise_volume : apu->noise_envelope_decay_counter;
        }

    } else {
        apu->divider--;
    }

    if (apu->triangle_time == 0) {
        apu->triangle_time = apu->triangle_timer;
        if (apu->triangle_linear_counter > 0 && apu->triangle_length > 0) {
            apu->triangle_sequence = (apu->triangle_sequence + 1) % 32;
            apu->triangle_value = triangle_sequencer[apu->triangle_sequence];
        }
    } else {
        apu->triangle_time--;
    }

    if (apu->frame_divider == 0) {
        apu->frame_divider = 1789773/240;

        // Skip frame 5 in mode 0
        if (apu->frame_counter_mode == 0 && apu->frame_counter == 4) {
            apu->frame_counter = 3;
        }

        // Every frame count
        if (apu->triangle_counter_reload) {
            apu->triangle_linear_counter = apu->triangle_counter_load;
        } else if (apu->triangle_linear_counter > 0) {
            apu->triangle_linear_counter--;
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

        if (apu->noise_envelope_start) {
            apu->noise_envelope_start = 0;
            apu->noise_envelope_decay_counter = 15;
            apu->noise_envelope_divider = apu->noise_volume;
        } else {
            if (apu->noise_envelope_divider == 0) {
                apu->noise_envelope_divider = apu->noise_volume;
                if (apu->noise_envelope_decay_counter == 0) {
                    if (apu->noise_counter_halt_envelope_loop) {
                        apu->noise_envelope_decay_counter = 15;
                    }
                } else {
                    apu->noise_envelope_decay_counter--;
                }
            } else {
                apu->noise_envelope_divider--;
            }
        }

        // Every other frame count
        if (apu->frame_counter % 2 == 1) {
            for (int i = 0; i < 2; i++) {
                if (apu->pulse[i].length != 0 && !apu->pulse[i].counter_halt_envelope_loop) {
                    apu->pulse[i].length--;
                }

                uint16_t change_amount = apu->pulse[i].timer >> apu->pulse[i].sweep_shift;
                if (apu->pulse[i].sweep_negate) {
                    // Channel 1 and 2 differ in how they negate the shift
                    change_amount = i == 0 ? ~change_amount : -change_amount;
                }

                uint16_t target_period = apu->pulse[i].timer + change_amount;

                if (target_period > 0x07ff) {
                    apu->pulse[i].sweep_mute = 1;
                }

                if (apu->pulse[i].sweep_counter == 0 && apu->pulse[i].sweep_enable && !apu->pulse[i].sweep_mute) {
                    apu->pulse[i].timer = target_period;
                }

                if (apu->pulse[i].sweep_counter == 0 || apu->pulse[i].sweep_reload) {
                    apu->pulse[i].sweep_reload = 0;
                    apu->pulse[i].sweep_counter = apu->pulse[i].sweep_period;
                } else {
                    apu->pulse[i].sweep_counter--;
                }
            }

            if (!apu->triangle_counter_halt) {
                apu->triangle_counter_reload = 0;
            }

            if (apu->triangle_length > 0 && !apu->triangle_counter_halt) {
                apu->triangle_length--;
            }

            if (apu->noise_length > 0) {
                apu->noise_length--;
            }
        }

        if (apu->frame_counter == 0) {
            apu->frame_counter = 4;
        } else {
            apu->frame_counter--;
        }
    } else {
        apu->frame_divider--;
    }

    apu->clocks++;
    while (apu->clocks >= 0) {
        while ((apu->buffer_tail + 1) % 2048 == apu->buffer_head);

        apu->clocks -= 1789773.0f / apu->sampling;
        uint8_t pulse_group = apu->pulse[0].value + apu->pulse[1].value;
        float pulse_out = pulse_group == 0 ? 0 : 95.88 / (8128.0 / pulse_group + 100);
        float tnd_group = apu->triangle_value / 8227.0 + (apu->noise_value / 12241.0) + 0;
        float tnd_out = tnd_group == 0 ? 0 : 159.79 / (1 / tnd_group + 100);
        apu->buffer[apu->buffer_tail] = pulse_out + tnd_out;
        apu->buffer_tail = (apu->buffer_tail + 1) % 2048;
    }
}
