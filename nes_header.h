struct nes_rom {
    int prg_rom_size;
    int chr_rom_size;

    int mirroring;
    int non_volatile_memory;
    int trainer;
    int four_screen_mode;
    int mapper_id;
    int submapper_id;

    uint8_t *prg_rom;
    uint8_t *chr_rom;
    uint8_t *prg_ram;
};

int read_rom(struct nes_rom *rom, char *filename);
