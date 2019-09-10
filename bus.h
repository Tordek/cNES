struct device {
    uint16_t start;
    uint16_t end;
    void *device;
    uint8_t (*read)(void *device, uint16_t address);
    void (*write)(void *device, uint16_t address, uint8_t data);
};

struct bus {
    struct device devices[10];
    int registered_devices;
};

uint8_t bus_read(struct bus *bus, uint16_t address);
void bus_write(struct bus *bus, uint16_t address, uint8_t contents);
void register_device(struct bus *bus, struct device device);
