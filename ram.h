extern uint8_t cpu_ram[];
uint8_t cpu_ram_read(void *ram, uint16_t address);
void cpu_ram_write(void *ram, uint16_t address, uint8_t data);
