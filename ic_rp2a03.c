#include <stdint.h>
#include <string.h>
#include <math.h>
#include "ic_rp2a03.h"

#define FREQ 48000.0

void ic_rp2a03_init(struct ic_rp2a03 *apu)
{
    *apu = (struct ic_rp2a03) {
        .pulse_1_duty = 0,
        .pulse_1_envelope = 0,
        .pulse_1_constant_volume = 0,
        .pulse_1_volume = 0,
        .pulse_1_sweep_shift = 0,
        .pulse_1_sweep_negate = 0,
        .pulse_1_sweep_period = 0,
        .pulse_1_sweep_enable = 0,
        .pulse_1_timer = 0,
        .pulse_1_length = 0,

        .pulse_1_time = 0,
        .pulse_1_value = 0,

        .pulse_2_duty = 0,
        .pulse_2_envelope = 0,
        .pulse_2_constant_volume = 0,
        .pulse_2_volume = 0,
        .pulse_2_sweep_shift = 0,
        .pulse_2_sweep_negate = 0,
        .pulse_2_sweep_period = 0,
        .pulse_2_sweep_enable = 0,
        .pulse_2_timer = 0,
        .pulse_2_length = 0,

        .pulse_2_time = 0,
        .pulse_2_value = 0,

        .buffer_start = 0,
        .buffer_count = 0,
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
        case 0x0000:
        {
            uint8_t duty = data >> 6;
            apu->pulse_1_duty = duty == 0 ? 0x40
                              : duty == 1 ? 0x60
                              : duty == 2 ? 0x78
                              : 0x9f;
            apu->pulse_1_envelope = (data & 0x20) >> 5;
            apu->pulse_1_constant_volume = (data & 0x10) >> 4;
            apu->pulse_1_volume = (data & 0x0f);
        }
            break;
        case 0x0001:
            apu->pulse_1_sweep_enable = (data & 0x80) >> 7;
            apu->pulse_1_sweep_period = (data & 0x70) >> 4;
            apu->pulse_1_sweep_negate = (data & 0x08) >> 3;
            apu->pulse_1_sweep_shift = (data & 0x07);
            break;
        case 0x0002:
            apu->pulse_1_timer &= 0xff00;
            apu->pulse_1_timer |= data;
            break;
        case 0x0003:
            apu->pulse_1_timer &= 0x00ff;
            apu->pulse_1_timer |= (data & 0x07) << 8;
            apu->pulse_1_time = 0;
            apu->pulse_1_length = (data & 0xf8) >> 3;
            break;
        case 0x0004:
        {
            uint8_t duty = data >> 6;
            apu->pulse_2_duty = duty == 0 ? 0x40
                              : duty == 1 ? 0x60
                              : duty == 2 ? 0x78
                              : 0x9f;
            apu->pulse_2_envelope = (data & 0x20) >> 5;
            apu->pulse_2_constant_volume = (data & 0x10) >> 4;
            apu->pulse_2_volume = (data & 0x0f);
        }
            break;
        case 0x0005:
            apu->pulse_2_sweep_enable = (data & 0x80) >> 7;
            apu->pulse_2_sweep_period = (data & 0x70) >> 4;
            apu->pulse_2_sweep_negate = (data & 0x08) >> 3;
            apu->pulse_2_sweep_shift = (data & 0x07);
            break;
        case 0x0006:
            apu->pulse_2_timer &= 0xff00;
            apu->pulse_2_timer |= data;
            break;
        case 0x0007:
            apu->pulse_2_timer &= 0x00ff;
            apu->pulse_2_timer |= (data & 0x07) << 8;
            apu->pulse_2_time = 0;
            apu->pulse_2_length = (data & 0xf8) >> 3;
            break;
    }
}

void ic_rp2a03_sdl_audio_callback(void *userdata, uint8_t *stream_raw, int len)
{
    struct ic_rp2a03 *apu_data = userdata;
    int flen = len / sizeof(float);
    float *stream = (float *)stream_raw;

    printf("%d - %d\n", flen, apu_data->buffer_count);
    if (apu_data->buffer_count < flen) {
        for (int i = 0; i < flen; i++) {
            stream[i] = 0.0f;
        }
        return;
    }
    for (int i = 0; i < flen; i++) {
        stream[i] = apu_data->buffer[apu_data->buffer_start];
        printf("%d, %f\n", i, stream[i]);
        apu_data->buffer_start = (apu_data->buffer_start + 1) % 1024;
        apu_data->buffer_count--;
    }
}

void ic_rp2a03_clock(struct ic_rp2a03 *apu)
{
    if (apu->divider == 0) {
        apu->divider = 1;

        if (apu->pulse_1_time == 0) {
            apu->pulse_1_time = apu->pulse_1_timer;

            if (apu->pulse_1_length > 0) {
                if (apu->pulse_1_duty & 0x80) {
                    apu->pulse_1_value = -0.01f;
                } else {
                    apu->pulse_1_value = 0.01f;
                }
                apu->pulse_1_duty = (apu->pulse_1_duty << 1) | (apu->pulse_1_duty >> 7);
            }
        } else {
            apu->pulse_1_time--;
        }

        if (apu->pulse_2_time == 0) {
            apu->pulse_2_time = apu->pulse_2_timer;

            if (apu->pulse_2_length > 0) {
                if (apu->pulse_2_duty & 0x80) {
                    apu->pulse_2_value = -0.01f;
                } else {
                    apu->pulse_2_value = 0.01f;
                }
                apu->pulse_2_duty = (apu->pulse_2_duty << 1) | (apu->pulse_2_duty >> 7);
            }
        } else {
            apu->pulse_2_time--;
        }
    } else {
        apu->divider--;
    }

    if (apu->frame_divider == 0) {
        apu->frame_divider = 357360;

        if (apu->pulse_1_length > 0)
            apu->pulse_1_length--;
        if (apu->pulse_2_length > 0)
            apu->pulse_2_length--;
    } else {
        apu->frame_divider--;
    }

    apu->clocks++;
    while (apu->clocks >= 0) {
        apu->clocks -= 1789773 / apu->sampling;
        apu->buffer[(apu->buffer_start + apu->buffer_count) % 1024] = apu->pulse_2_value + apu->pulse_1_value;
        apu->buffer_count++;
    }
}