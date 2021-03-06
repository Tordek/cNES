struct ic_rp2a03_pulse_channel {
    uint8_t duty;
    uint8_t counter_halt_envelope_loop;
    uint8_t constant_volume;
    uint8_t volume;
    uint8_t sweep_shift;
    uint8_t sweep_negate;
    uint8_t sweep_period;
    uint8_t sweep_enable;
    int sweep_mute;
    int sweep_reload;
    uint16_t timer;
    uint8_t length;

    int envelope_start;
    uint8_t envelope_decay_counter;
    uint8_t envelope_divider;
    uint8_t sweep_counter;
    uint8_t duty_counter;
    uint16_t time;
    uint8_t value;
};

struct ic_rp2a03 {
    struct ic_rp2a03_pulse_channel pulse[2];

    uint8_t triangle_counter_halt;
    uint8_t triangle_counter_load;
    uint16_t triangle_timer;
    uint16_t triangle_time;
    uint8_t triangle_length;
    uint8_t triangle_value;
    uint8_t triangle_sequence;
    int triangle_linear_counter;
    int triangle_counter_reload;

    uint8_t noise_counter_halt_envelope_loop;
    uint8_t noise_constant_volume;
    uint8_t noise_volume;
    uint8_t noise_mode;
    uint8_t noise_timer;
    uint8_t noise_time;
    uint8_t noise_counter_load;
    uint8_t noise_length;
    int noise_envelope_start;
    uint16_t noise_lfsr;
    uint8_t noise_value;
    uint8_t noise_envelope_divider;
    uint8_t noise_envelope_decay_counter;

    uint8_t dmc_irq_enable;
    uint8_t dmc_loop;
    uint8_t dmc_frequency;
    uint8_t dmc_load_counter;
    uint8_t dmc_sample_address;
    uint8_t dmc_sample_length;

    int frame_counter_mode;

    float clk;

    int divider;
    int frame_divider;
    int frame_counter;
};

uint8_t ic_rp2a03_read(struct ic_rp2a03 *apu, uint16_t address);
void ic_rp2a03_write(struct ic_rp2a03 *apu, uint16_t address, uint8_t data);
float ic_rp2a03_clock(struct ic_rp2a03 *apu);
void ic_rp2a03_init(struct ic_rp2a03 *apu);
