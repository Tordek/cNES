struct controllers {
    uint8_t p1_status;
    uint8_t p2_status;
    uint8_t p1_latch;
    uint8_t p1_strobe;
    uint8_t p2_strobe;
};

uint8_t controllers_read(struct controllers *controllers, uint16_t address);
void controllers_write(struct controllers *controllers, uint16_t address, uint8_t data);
int controllers_handle(struct controllers *controllers, SDL_Event *event);
void controllers_init(struct controllers *controllers);
