struct ic_rp2a03 {
    uint8_t pulse_1_duty;
    uint8_t pulse_1_envelope;
    uint8_t pulse_1_constant_volume;
    uint8_t pulse_1_volume;
    uint8_t pulse_1_sweep_shift;
    uint8_t pulse_1_sweep_negate;
    uint8_t pulse_1_sweep_period;
    uint8_t pulse_1_sweep_enable;
    uint16_t pulse_1_timer;
    uint8_t pulse_1_length;

    uint16_t pulse_1_time;
    float pulse_1_value;

    uint8_t pulse_2_duty;
    uint8_t pulse_2_envelope;
    uint8_t pulse_2_constant_volume;
    uint8_t pulse_2_volume;
    uint8_t pulse_2_sweep_shift;
    uint8_t pulse_2_sweep_negate;
    uint8_t pulse_2_sweep_period;
    uint8_t pulse_2_sweep_enable;
    uint16_t pulse_2_timer;
    uint8_t pulse_2_length;

    uint16_t pulse_2_time;
    float pulse_2_value;

    float buffer[4096];
    volatile int buffer_start;
    volatile int buffer_count;
    int buffer_max;
    float clocks;
    float clk;

    float sampling;
    int divider;
    int frame_divider;

    SDL_bool buffer_fool;
    SDL_cond *buffer_full_cond;
    SDL_mutex *buffer_full_lock;
};

uint8_t ic_rp2a03_read(struct ic_rp2a03 *apu, uint16_t address);
void ic_rp2a03_write(struct ic_rp2a03 *apu, uint16_t address, uint8_t data);
void ic_rp2a03_clock(struct ic_rp2a03 *apu);
void ic_rp2a03_init(struct ic_rp2a03 *apu);

void ic_rp2a03_sdl_audio_callback(void *userdata, uint8_t *stream, int len);
