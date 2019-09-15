struct mapper {
    struct ic_6502_registers *cpu;
    struct ic_2C02_registers *ppu;
    int clock;
    struct apu *apu;
    uint8_t (*cpu_bus_read)(struct mapper *device, uint16_t address);
    void (*cpu_bus_write)(struct mapper *device, uint16_t address, uint8_t data);
    uint8_t (*ppu_bus_read)(struct mapper *device, uint16_t address);
    void (*ppu_bus_write)(struct mapper *device, uint16_t address, uint8_t data);

    int16_t dma_page;
    int16_t dma_write_time;
};
