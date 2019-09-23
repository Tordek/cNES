struct mapper {
    int clock;
    struct ic_6502_registers *cpu;
    struct ic_2C02_registers *ppu;
    struct ic_rp2a03 *apu;
    struct controllers *controllers;
    uint8_t (*cpu_bus_read)(struct mapper *device, uint16_t address);
    void (*cpu_bus_write)(struct mapper *device, uint16_t address, uint8_t data);
    uint8_t (*ppu_bus_read)(struct mapper *device, uint16_t address);
    void (*ppu_bus_write)(struct mapper *device, uint16_t address, uint8_t data);

    int16_t dma_page;
    int16_t dma_write_time;
};

int mapper_clock(struct mapper *mapper);
