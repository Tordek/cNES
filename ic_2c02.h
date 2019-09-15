struct oam_value {
    uint8_t y;
    uint8_t tile_index;
    uint8_t attributes;
    uint8_t x;
};

enum ic_2c02_status_flags {
    ic_2c02_status_sprite_overflow = 0x20,
    ic_2c02_status_sprite_0_hit = 0x40,
    ic_2c02_status_vblank = 0x80,
};

struct ic_2C02_registers {
    uint8_t status;

    int do_nmi;

    int clock;
    uint8_t enable;

    int pixel;
    int scanline;

    // Background registers
    uint16_t vram_increment;
    uint16_t vram_address;
    uint16_t t;
    uint16_t fine_x;
    int w; // Read latch
    uint16_t pattern_hi;
    uint16_t pattern_lo;
    uint8_t palette;
    uint8_t palette_next;
    uint16_t background;

    uint16_t pattern_hi_next;
    uint16_t pattern_lo_next;
    uint16_t nametable_byte_next;
    uint8_t attribute_byte;
    uint8_t attribute_byte_next;

    // Sprites
    uint8_t sprite_pattern_table;
    union {
        uint8_t raw[256];
        struct oam_value data[64];
    } primary_oam;
    union {
        uint8_t raw[32];
        struct oam_value data[8];
    } secondary_oam;
    uint8_t sprite_pattern_hi[8];
    uint8_t sprite_pattern_lo[8];
    uint8_t sprite_attributes[8];
    uint8_t sprite_id[8];
    uint8_t sprite_x[8];
    int secondary_oam_write_enable;
    int tall_sprites;

    uint8_t secondary_oam_index;
    uint8_t n;

    uint8_t oam_addr;
    int oam_state;

    int m;
    int waste;

    uint8_t ppudata_read;

    struct mapper *mapper;
};

void ic_2C02_clock(struct ic_2C02_registers *ppu, SDL_Surface *surface);
uint8_t ic_2c02_read(struct ic_2C02_registers *device, uint16_t address);
void ic_2c02_write(struct ic_2C02_registers *device, uint16_t address, uint8_t value);
void ic_2c02_reset(struct ic_2C02_registers *ppu);
