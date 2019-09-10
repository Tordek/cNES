struct ic_6502_registers {
    uint16_t program_counter;
    uint8_t status;
    uint8_t stack_pointer;
    uint8_t a;
    uint8_t x;
    uint8_t y;

    uint8_t cycles;
    struct mapper *mapper;
};

extern uint8_t const ic_6502_op_length[];
void ic_6502_reset(struct ic_6502_registers * restrict registers);
void ic_6502_clock(struct ic_6502_registers * restrict registers);
void ic_6502_nmi(struct ic_6502_registers * restrict registers);
