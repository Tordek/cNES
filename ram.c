#include <stdint.h>
#include "ram.h"

uint8_t cpu_ram[0x0800] = {0};

uint8_t cpu_ram_read(void *ram, uint16_t address)
{
    uint8_t *_ram = (uint8_t *)ram;
    return _ram[address & 0x7FF];
}

void cpu_ram_write(void *ram, uint16_t address, uint8_t data)
{
    uint8_t *_ram = (uint8_t *)ram;
    _ram[address & 0x7FF] = data;
}

