struct ic_2C02_registers {
    uint8_t status;
    uint8_t nametable_byte;
    uint8_t attribute_byte;
    uint16_t nametable;
    uint16_t pattern_hi;
    uint16_t pattern_lo;
    uint8_t palette;

    uint16_t pattern_hi_next;
    uint16_t pattern_lo_next;
    uint16_t nametable_byte_next;
    uint8_t attribute_byte_next;

    uint16_t vram_address;
    int do_nmi;

    uint8_t scroll_x;
    uint8_t scroll_y;

    int scroll_v;
    int addr_latch;
    int clock;

    uint16_t background;
    uint16_t vram_increment;

    struct mapper *mapper;
};

void ic_2C02_clock(struct ic_2C02_registers *ppu, SDL_Surface *surface);
uint8_t ic_2c02_read(void *device, uint16_t address);
void ic_2c02_write(void *device, uint16_t address, uint8_t value);
void ic_2c02_reset(struct ic_2C02_registers *ppu);
