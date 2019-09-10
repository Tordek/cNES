#include <stdint.h>
#include "bus.h"

void register_device(struct bus *bus, struct device device) {
    bus->devices[bus->registered_devices] = device;
    bus->registered_devices++;
}

uint8_t bus_read(struct bus *bus, uint16_t address) {
    for (int i = 0; i < bus->registered_devices; i++) {
        if (bus->devices[i].start <= address && address <= bus->devices[i].end) {
            return bus->devices[i].read(bus->devices[i].device, address);
        }
    }
    return 0x54;
}

void bus_write(struct bus *bus, uint16_t address, uint8_t contents) {
    for (int i = 0; i < bus->registered_devices; i++) {
        if (bus->devices[i].start <= address && address <= bus->devices[i].end) {
            bus->devices[i].write(bus->devices[i].device, address, contents);
            return;
        }
    }
}
