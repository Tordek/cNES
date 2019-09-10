struct mapper {
    struct ic_6502_registers *cpu;
    struct ic_2C02_registers *ppu;
    struct apu *apu;
    uint8_t (*cpu_bus_read)(void *device, uint16_t address);
    void (*cpu_bus_write)(void *device, uint16_t address, uint8_t data);
    uint8_t (*ppu_bus_read)(void *device, uint16_t address);
    void (*ppu_bus_write)(void *device, uint16_t address, uint8_t data);
};

extern struct mapper * (*mappers[])(struct nes_rom *rom);
