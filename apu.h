struct apu {};

uint8_t apu_read(struct apu *apu, uint16_t address);

void apu_write(struct apu *apu, uint16_t address, uint8_t data);
