#include <stdint.h>
#include "mapper.h"
#include "ic_6502.h"

#define STACK_BASE 0x0100

#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR 0xFFFE

enum ic_6502_flags {
    ic_6502_C = 1 << 0,
    ic_6502_Z = 1 << 1,
    ic_6502_I = 1 << 2,
    ic_6502_D = 1 << 3,
    ic_6502_B = 1 << 4,
    ic_6502_U = 1 << 5,
    ic_6502_V = 1 << 6,
    ic_6502_N = 1 << 7,
};

uint8_t const ic_6502_op_cycles[] = {
    7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

void ic_6502_init(struct ic_6502_registers *cpu)
{
    *cpu = (struct ic_6502_registers) {
        .program_counter = 0,
        .status = 0,
        .stack_pointer = 0,
        .a = 0,
        .x = 0,
        .y = 0,

        .cycles = 0,
        .mapper = NULL,
    };
}

inline void update_flag(struct ic_6502_registers * restrict registers, enum ic_6502_flags flag, int value)
{
    if (value) {
        registers->status |= flag;
    } else {
        registers->status &= ~flag;
    }
}

void ic_6502_clock(struct ic_6502_registers * restrict registers)
{
    if (registers->cycles == 0) {
        uint8_t opcode = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter++);

        registers->cycles = ic_6502_op_cycles[opcode];

        int8_t addr_rel;
        uint16_t addr_abs;
        uint8_t fetched;
        uint8_t extra_cycle = 0;

        // Handle addressing mode
        switch (opcode) {
            case 0x00: case 0x08: case 0x18: case 0x1a: case 0x28: case 0x38: case 0x3a: case 0x40:
            case 0x48: case 0x58: case 0x5a: case 0x60: case 0x68: case 0x78: case 0x7a: case 0x88:
            case 0x8a: case 0x98: case 0x9a: case 0xa8: case 0xaa: case 0xb8: case 0xba: case 0xc8:
            case 0xca: case 0xd8: case 0xda: case 0xe8: case 0xea: case 0xf8: case 0xfa:
                // IMP
                break;

            case 0x0a: case 0x2a: case 0x4a: case 0x6a:
                // ACC
                break;

            case 0x02: case 0x09: case 0x0b: case 0x12: case 0x22: case 0x29: case 0x2b: case 0x32:
            case 0x42: case 0x49: case 0x4b: case 0x52: case 0x62: case 0x69: case 0x6b: case 0x72:
            case 0x80: case 0x82: case 0x89: case 0x8b: case 0x92: case 0xa0: case 0xa2: case 0xa9:
            case 0xab: case 0xb2: case 0xc0: case 0xc2: case 0xc9: case 0xcb: case 0xd2: case 0xe0:
            case 0xe2: case 0xe9: case 0xeb: case 0xf2:
                // IMM
                addr_abs = registers->program_counter;
                registers->program_counter++;
                break;

            case 0x04: case 0x05: case 0x06: case 0x07: case 0x24: case 0x25: case 0x26: case 0x27:
            case 0x44: case 0x45: case 0x46: case 0x47: case 0x64: case 0x65: case 0x66: case 0x67:
            case 0x84: case 0x85: case 0x86: case 0x87: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
            case 0xc4: case 0xc5: case 0xc6: case 0xc7: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
                // ZP0
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;
                break;

            case 0x14: case 0x15: case 0x16: case 0x17: case 0x34: case 0x35: case 0x36: case 0x37:
            case 0x54: case 0x55: case 0x56: case 0x57: case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x94: case 0x95: case 0xb4: case 0xb5: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
            case 0xf4: case 0xf5: case 0xf6: case 0xf7:
                // ZPX
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter) + registers->x & 0xFF;
                registers->program_counter++;
                break;

            case 0x96: case 0x97: case 0xb6: case 0xb7:
                // ZPY
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter) + registers->y & 0xFF;
                registers->program_counter++;
                break;

            case 0x10: case 0x30: case 0x50: case 0x70: case 0x90: case 0xb0: case 0xd0: case 0xf0:
                // REL
                addr_rel = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;
                break;

            case 0x0c: case 0x0d: case 0x0e: case 0x0f: case 0x20: case 0x2c: case 0x2d: case 0x2e:
            case 0x2f: case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x6d: case 0x6e: case 0x6f:
            case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0xac: case 0xad: case 0xae: case 0xaf:
            case 0xcc: case 0xcd: case 0xce: case 0xcf: case 0xec: case 0xed: case 0xee: case 0xef:
                // ABS
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;
                addr_abs |= registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter) << 8;
                registers->program_counter++;
                break;

            case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
            case 0x5c: case 0x5d: case 0x5e: case 0x5f: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
            case 0x9c: case 0x9d: case 0xbc: case 0xbd: case 0xdc: case 0xdd: case 0xde: case 0xdf:
            case 0xfc: case 0xfd: case 0xfe: case 0xff:
            {
                // ABX
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;
                uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;

                addr_abs |= hi << 8;
                addr_abs += registers->x;
                if (addr_abs >> 8 != hi) {
                    extra_cycle = 1;
                }
            }
                break;

            case 0x19: case 0x1b: case 0x39: case 0x3b: case 0x59: case 0x5b: case 0x79: case 0x7b:
            case 0x99: case 0x9b: case 0x9e: case 0x9f: case 0xb9: case 0xbb: case 0xbe: case 0xbf:
            case 0xd9: case 0xdb: case 0xf9: case 0xfb:
            {
                // ABY
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;
                uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;

                addr_abs |= hi << 8;
                addr_abs += registers->y;
                if (addr_abs >> 8 != hi) {
                    extra_cycle = 1;
                }
            }
                break;

            case 0x6c:
            {
                // IND (Handles page boundary bug)
                uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;
                uint16_t hi = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter) << 8;
                registers->program_counter++;

                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, hi | lo);
                lo++;
                addr_abs |= registers->mapper->cpu_bus_read(registers->mapper, hi | lo) << 8;
            }

                break;

            case 0x01: case 0x03: case 0x21: case 0x23: case 0x41: case 0x43: case 0x61: case 0x63:
            case 0x81: case 0x83: case 0xa1: case 0xa3: case 0xc1: case 0xc3: case 0xe1: case 0xe3:
            {
                // IZX
                uint8_t t = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter) + registers->x;
                registers->program_counter++;

                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, t);
                t++;
                addr_abs |= registers->mapper->cpu_bus_read(registers->mapper, t) << 8;
            }

                break;

            case 0x11: case 0x13: case 0x31: case 0x33: case 0x51: case 0x53: case 0x71: case 0x73:
            case 0x91: case 0x93: case 0xb1: case 0xb3: case 0xd1: case 0xd3: case 0xf1: case 0xf3:
            {
                // IZY
                uint8_t t = registers->mapper->cpu_bus_read(registers->mapper, registers->program_counter);
                registers->program_counter++;

                uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, t);
                t++;
                uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, t);
                addr_abs = (hi << 8 | lo) + registers->y;

                if (addr_abs >> 8 != hi) {
                    extra_cycle = 1;
                }
            }

                break;
        }

        // Handle operation
        switch (opcode) {
            case 0x00:
            {
                // BRK
                registers->program_counter++;

                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter >> 8);
                registers->stack_pointer--;
                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter & 0xFF);
                registers->stack_pointer--;
                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->status | ic_6502_B);
                registers->stack_pointer--;
                registers->status |= ic_6502_I;

                uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, IRQ_VECTOR);
                uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, IRQ_VECTOR + 1);

                registers->program_counter = hi << 8 | lo;
            }
                break;

            case 0x01: case 0x05: case 0x09: case 0x0d: case 0x11: case 0x15: case 0x19: case 0x1d:
                // ORA
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a |= fetched;
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);

                registers->cycles += extra_cycle;
                break;

            case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52: case 0x62: case 0x72:
            case 0x92: case 0xb2: case 0xd2: case 0xf2:
                // KIL - Illegal
                break;

            case 0x03: case 0x07: case 0x0f: case 0x13: case 0x17: case 0x1b: case 0x1f:
            {
                // SLO - Illegal
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = fetched << 1;
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);

                registers->a |= temp;
                update_flag(registers, ic_6502_C, fetched & 0x80);
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
            }
                break;

            case 0x1c: case 0x3c: case 0x5c: case 0x7c: case 0xdc: case 0xfc:
                // NOP (extra cycles)
                registers->cycles += extra_cycle;
                break;

            case 0x04: case 0x0c: case 0x14: case 0x1a: case 0x34: case 0x3a: case 0x44: case 0x54:
            case 0x5a: case 0x64: case 0x74: case 0x7a: case 0x80: case 0x82: case 0x89: case 0xc2:
            case 0xd4: case 0xda: case 0xe2: case 0xea: case 0xf4: case 0xfa:
                // NOP
                break;

            case 0x0a:
            {
                // ASL A
                fetched = registers->a;

                uint16_t temp = fetched << 1;
                update_flag(registers, ic_6502_C, temp & 0xFF00);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_N, temp & 0x0080);

                registers->a = temp;
            }
                break;

            case 0x06: case 0x0e: case 0x16: case 0x1e:
            {
                // ASL
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);

                uint16_t temp = fetched << 1;
                update_flag(registers, ic_6502_C, temp & 0xFF00);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_N, temp & 0x0080);

                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);
            }
                break;

            case 0x08:
                // PHP
                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->status | ic_6502_B | ic_6502_U);
                registers->stack_pointer--;
                registers->status &= ~ic_6502_B;
                registers->status |= ic_6502_U;

                break;

            case 0x0b: case 0x2b:
                // ANC
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a &= fetched;
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
                update_flag(registers, ic_6502_C, registers->a & 0x80);
                break;

            case 0x10:
                // BPL
                if ((registers->status & ic_6502_N) == 0) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0x18:
                // CLC
                registers->status &= ~ic_6502_C;
                break;

            case 0x20:
                // JSR
                registers->program_counter--;

                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter >> 8);
                registers->stack_pointer--;
                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter  & 0xFF);
                registers->stack_pointer--;

                registers->program_counter = addr_abs;

                break;

            case 0x21: case 0x25: case 0x29: case 0x2d: case 0x31: case 0x35: case 0x39: case 0x3d:
                // AND
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a &= fetched;
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);

                registers->cycles += extra_cycle;

                break;

            case 0x23: case 0x27: case 0x2f: case 0x33: case 0x37: case 0x3b: case 0x3f:
            {
                // RLA
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = fetched << 1 | (registers->status & 0x01);
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);

                registers->a &= temp;
                update_flag(registers, ic_6502_C, fetched & 0x80);
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
            }
                break;

            case 0x24: case 0x2c:
            {
                // BIT
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = registers->a & fetched;
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, fetched & 0x80);
                update_flag(registers, ic_6502_V, fetched & 0x40);
            }
                break;

            case 0x2a:
            {
                // ROL A
                fetched = registers->a;

                uint8_t temp = fetched << 1 | (registers->status & 0x01);
                update_flag(registers, ic_6502_C, fetched & 0x80);
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);

                registers->a = temp;
            }
                break;

            case 0x26: case 0x2e: case 0x36: case 0x3e:
            {
                // ROL
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);

                uint8_t temp = fetched << 1 | (registers->status & 0x01);
                update_flag(registers, ic_6502_C, fetched & 0x80);
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);

                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);
            }
                break;

            case 0x28:
                // PLP
                registers->stack_pointer++;
                registers->status = registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer);
                registers->status &= ~ic_6502_B;
                registers->status |= ic_6502_U;

                break;

            case 0x30:
                // BMI
                if (registers->status & ic_6502_N) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0x38:
                // SEC
                registers->status |= ic_6502_C;
                break;

            case 0x40:
            {
                // RTI
                registers->stack_pointer++;
                registers->status = registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer);
                registers->status &= ~ic_6502_B & ~ic_6502_U;

                registers->stack_pointer++;
                uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer);

                registers->stack_pointer++;
                uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer);
                registers->program_counter = hi << 8 | lo;
            }
                break;

            case 0x41: case 0x45: case 0x49: case 0x4d: case 0x51: case 0x55: case 0x59: case 0x5d:
                // EOR
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a ^= fetched;
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);

                registers->cycles += extra_cycle;
                break;

            case 0x43: case 0x47: case 0x4f: case 0x53: case 0x57: case 0x5b: case 0x5f:
            {
                // SRE
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = fetched >> 1;
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);

                registers->a ^= temp;
                update_flag(registers, ic_6502_C, fetched & 0x01);
                update_flag(registers, ic_6502_Z, registers->a == 0x00);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
            }
                break;

            case 0x4a:
            {
                // LSR A
                fetched = registers->a;

                update_flag(registers, ic_6502_C, fetched & 0x01);
                uint8_t temp = fetched >> 1;
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);

                registers->a = temp;
            }
                break;

            case 0x46: case 0x4e: case 0x56: case 0x5e:
            {
                // LSR
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);

                update_flag(registers, ic_6502_C, fetched & 0x01);
                uint8_t temp = fetched >> 1;
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);

                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);
            }
                break;

            case 0x48:
                // PHA
                registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->a);
                registers->stack_pointer--;
                break;

            case 0x4b:
            {
                // ALR
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = registers->a & fetched;
                update_flag(registers, ic_6502_C, temp & 0x01);
                temp >>= 1;
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);
                registers->a = temp;
            }
                break;

            case 0x4c: case 0x6c:
                // JMP
                registers->program_counter = addr_abs;
                break;

            case 0x50:
                // BVC
                if ((registers->status & ic_6502_V) == 0) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0x58:
                // CLI
                registers->status &= ~ic_6502_I;
                break;

            case 0x60:
                // RTS
                registers->stack_pointer++;
                addr_abs = registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer);
                registers->stack_pointer++;
                addr_abs |= registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer) << 8;
                registers->program_counter = addr_abs + 1;
                break;

            case 0x61: case 0x65: case 0x69: case 0x6d: case 0x71: case 0x75: case 0x79: case 0x7d:
            {
                // ADC
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint16_t temp = fetched + registers->a + (registers->status & ic_6502_C);
                update_flag(registers, ic_6502_C, temp & 0xFF00);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_V, ~(registers->a ^ fetched) & (registers->a ^ temp) & 0x80);
                update_flag(registers, ic_6502_N, temp & 0x0080);
                registers->a = temp;

                registers->cycles += extra_cycle;
            }
                break;

            case 0x63: case 0x67: case 0x6f: case 0x73: case 0x77: case 0x7b: case 0x7f:
            {
                // RRA
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t val = fetched >> 1 | registers->status << 7;
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, val);

                uint16_t temp = registers->a + val + (fetched & 0x01);
                update_flag(registers, ic_6502_C, temp & 0xFF00);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_V, ~(registers->a ^ val) & (registers->a ^ temp) & 0x80);
                update_flag(registers, ic_6502_N, temp & 0x0080);
                registers->a = temp;
            }
                break;

            case 0x6a:
            {
                // ROR A
                fetched = registers->a;

                uint8_t temp = fetched >> 1 | registers->status << 7;
                update_flag(registers, ic_6502_C, fetched & 0x01);
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);

                registers->a = temp;
            }
                break;

            case 0x66: case 0x6e: case 0x76: case 0x7e:
            {
                // ROR
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);

                uint8_t temp = fetched >> 1 | registers->status << 7;
                update_flag(registers, ic_6502_C, fetched & 0x01);
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);

                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);
            }
                break;

            case 0x68:
                // PLA
                registers->stack_pointer++;
                registers->a = registers->mapper->cpu_bus_read(registers->mapper, STACK_BASE + registers->stack_pointer);
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
                break;

            case 0x6b:
                // ARR
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a = (registers->status & ic_6502_C) << 7 | (registers->a & fetched) >> 1;

                update_flag(registers, ic_6502_C, registers->a & 0x40);
                update_flag(registers, ic_6502_V, (registers->a & 0x20) << 1 != (registers->a & 0x40));
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
                break;

            case 0x70:
                // BVS
                if (registers->status & ic_6502_V) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0x78:
                // SEI
                registers->status |= ic_6502_I;
                break;

            case 0x81: case 0x85: case 0x8d: case 0x91: case 0x95: case 0x99: case 0x9d:
                // STA
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, registers->a);

                break;

            case 0x83: case 0x87: case 0x8f: case 0x97:
                // SAX
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, registers->a & registers->x);
                break;

            case 0x84: case 0x8c: case 0x94:
                // STY
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, registers->y);

                break;

            case 0x86: case 0x8e: case 0x96:
                // STX
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, registers->x);

                break;

            case 0x88:
                // DEY
                registers->y--;
                update_flag(registers, ic_6502_Z, registers->y == 0);
                update_flag(registers, ic_6502_N, registers->y & 0x80);
                break;

            case 0x8a:
                // TXA
                registers->a = registers->x;
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
                break;

            case 0x8b:
                // XAA
                break;

            case 0x90:
                // BCC
                if ((registers->status & ic_6502_C) == 0) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0x93: case 0x9f:
                // AHX
                break;

            case 0x98:
                // TYA
                registers->a = registers->y;
                update_flag(registers, ic_6502_Z, registers->a == 0);
                update_flag(registers, ic_6502_N, registers->a & 0x80);
                break;

            case 0x9a:
                // TXS
                registers->stack_pointer = registers->x;
                break;

            case 0x9b:
                // TAS
                break;

            case 0x9c:
                // SHY
                break;

            case 0x9e:
                // SHX
                break;

            case 0xa0: case 0xa4: case 0xac: case 0xb4: case 0xbc:
                // LDY
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->y = fetched;
                update_flag(registers, ic_6502_Z, fetched == 0);
                update_flag(registers, ic_6502_N, (fetched & 0x80) == 0x80);

                registers->cycles += extra_cycle;
                break;

            case 0xa1: case 0xa5: case 0xa9: case 0xad: case 0xb1: case 0xb5: case 0xb9: case 0xbd:
                // LDA
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a = fetched;
                update_flag(registers, ic_6502_Z, fetched == 0);
                update_flag(registers, ic_6502_N, (fetched & 0x80) == 0x80);

                registers->cycles += extra_cycle;
                break;

            case 0xa2: case 0xa6: case 0xae: case 0xb6: case 0xbe:
                // LDX
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->x = fetched;
                update_flag(registers, ic_6502_Z, fetched == 0);
                update_flag(registers, ic_6502_N, (fetched & 0x80) == 0x80);

                registers->cycles += extra_cycle;
                break;

            case 0xa3: case 0xa7: case 0xab: case 0xaf: case 0xb3: case 0xb7: case 0xbf:
                // LAX
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                registers->a = fetched;
                registers->x = fetched;
                update_flag(registers, ic_6502_Z, registers->a == 0x00);
                update_flag(registers, ic_6502_N, registers->a & 0x80);

                registers->cycles += extra_cycle;
                break;

            case 0xa8:
                // TAY
                registers->y = registers->a;
                update_flag(registers, ic_6502_Z, registers->y == 0);
                update_flag(registers, ic_6502_N, registers->y & 0x80);
                break;

            case 0xaa:
                // TAX
                registers->x = registers->a;
                update_flag(registers, ic_6502_Z, registers->x == 0);
                update_flag(registers, ic_6502_N, registers->x & 0x80);
                break;

            case 0xb0:
                // BCS
                if (registers->status & ic_6502_C) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0xb8:
                // CLV
                registers->status &= ~ic_6502_V;
                break;

            case 0xba:
                // TSX
                registers->x = registers->stack_pointer;
                update_flag(registers, ic_6502_Z, registers->x == 0);
                update_flag(registers, ic_6502_N, registers->x & 0x80);
                break;

            case 0xbb:
                // LAS
                break;

            case 0xc0: case 0xc4: case 0xcc:
            {
                // CPY
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint16_t temp = registers->y - fetched;
                update_flag(registers, ic_6502_C, registers->y >= fetched);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_N, temp & 0x0080);
            }
                break;

            case 0xc1: case 0xc5: case 0xc9: case 0xcd: case 0xd1: case 0xd5: case 0xd9: case 0xdd:
            {
                // CMP
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint16_t temp = registers->a - fetched;
                update_flag(registers, ic_6502_C, registers->a >= fetched);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_N, temp & 0x0080);
            }
                break;

            case 0xc3: case 0xc7: case 0xcf: case 0xd3: case 0xd7: case 0xdb: case 0xdf:
            {
                // DCP
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = fetched - 1;
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);

                temp = registers->a - temp;
                update_flag(registers, ic_6502_C, registers->a >= temp);
                update_flag(registers, ic_6502_Z, temp == 0);
                update_flag(registers, ic_6502_N, temp & 0x80);
            }
                break;

            case 0xc6: case 0xce: case 0xd6: case 0xde:
                // DEC
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                fetched--;
                update_flag(registers, ic_6502_Z, fetched == 0);
                update_flag(registers, ic_6502_N, fetched & 0x80);
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, fetched);
                break;

            case 0xc8:
                // INY
                registers->y++;
                update_flag(registers, ic_6502_Z, registers->y == 0);
                update_flag(registers, ic_6502_N, registers->y & 0x80);
                break;

            case 0xca:
                // DEX
                registers->x--;
                update_flag(registers, ic_6502_Z, registers->x == 0);
                update_flag(registers, ic_6502_N, registers->x & 0x80);
                break;

            case 0xcb:
            {
                // AXS
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint16_t temp = (registers->a & registers->x) ;
                update_flag(registers, ic_6502_C, temp >= fetched);
                registers->x = temp - fetched;
                update_flag(registers, ic_6502_Z, registers->x == 0);
                update_flag(registers, ic_6502_N, registers->x & 0x80);
            }
                break;

            case 0xd0:
                // BNE
                if ((registers->status & ic_6502_Z) == 0) {
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->cycles++;
                    registers->program_counter = addr_abs;
                }
                break;

            case 0xd8:
                // CLD
                registers->status &= ~ic_6502_D;
                break;

            case 0xe0: case 0xe4: case 0xec:
            {
                // CPX
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint16_t temp = registers->x - fetched;
                update_flag(registers, ic_6502_C, registers->x >= fetched);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_N, temp & 0x0080);
            }
                break;

            case 0xe1: case 0xe5: case 0xe9: case 0xeb: case 0xed: case 0xf1: case 0xf5: case 0xf9:
            case 0xfd:
            {
                // SBC
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint16_t temp = (fetched ^ 0x00FF) + registers->a + (registers->status & ic_6502_C);
                update_flag(registers, ic_6502_C, temp & 0xFF00);
                update_flag(registers, ic_6502_Z, (temp & 0x00FF) == 0);
                update_flag(registers, ic_6502_V, (registers->a ^ fetched) & (registers->a ^ temp) & 0x80);
                update_flag(registers, ic_6502_N, temp & 0x0080);
                registers->a = temp;

                registers->cycles += extra_cycle;
            }
                break;

            case 0xe3: case 0xe7: case 0xef: case 0xf3: case 0xf7: case 0xfb: case 0xff:
            {
                // ISB
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                uint8_t temp = fetched + 1;
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, temp);

                uint8_t value = ~temp;
                temp = value + registers->a + (registers->status & 0x01);
                update_flag(registers, ic_6502_C, registers->a >= temp);
                update_flag(registers, ic_6502_V, (value ^ temp) & (registers->a ^ temp) & 0x80);
                update_flag(registers, ic_6502_Z, temp == 0x00);
                update_flag(registers, ic_6502_N, temp & 0x80);
                registers->a = temp;
            }
                break;

            case 0xe6: case 0xee: case 0xf6: case 0xfe:
                // INC
                fetched = registers->mapper->cpu_bus_read(registers->mapper, addr_abs);
                fetched++;
                update_flag(registers, ic_6502_Z, fetched == 0);
                update_flag(registers, ic_6502_N, fetched & 0x80);
                registers->mapper->cpu_bus_write(registers->mapper, addr_abs, fetched);
                break;

            case 0xe8:
                // INX
                registers->x++;
                update_flag(registers, ic_6502_Z, registers->x == 0);
                update_flag(registers, ic_6502_N, registers->x & 0x80);
                break;

            case 0xf0:
                // BEQ
                if (registers->status & ic_6502_Z) {
                    registers->cycles++;
                    addr_abs = registers->program_counter + addr_rel;
                    if ((addr_abs & 0xFF00) != (registers->program_counter & 0xFF00)) {
                        registers->cycles++;
                    }
                    registers->program_counter = addr_abs;
                }
                break;

            case 0xf8:
                // SED
                registers->status |= ic_6502_D;
                break;
        }
        registers->status |= ic_6502_U;
    }

    registers->cycles--;
}

void ic_6502_reset(struct ic_6502_registers *registers)
{
    uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, RESET_VECTOR);
    uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, RESET_VECTOR + 1);

    registers->program_counter = hi << 8 | lo;
    registers->a = 0;
    registers->x = 0;
    registers->y = 0;
    registers->stack_pointer = 0xFD;
    registers->status = 0x00 | ic_6502_U | ic_6502_I;

    registers->cycles = 8;
}

void ic_6502_nmi(struct ic_6502_registers * restrict registers)
{
    registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter >> 8);
    registers->stack_pointer--;
    registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter & 0xFF);
    registers->stack_pointer--;

    registers->status &= ~ic_6502_B;
    registers->status |= ic_6502_U | ic_6502_I;

    registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->status);
    registers->stack_pointer--;

    uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, NMI_VECTOR);
    uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, NMI_VECTOR + 1);

    registers->program_counter = hi << 8 | lo;
    registers->cycles = 8;
}

void ic_6502_irq(struct ic_6502_registers * restrict registers)
{
    if (registers->status & ic_6502_I) {
        return;
    }

    registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter >> 8);
    registers->stack_pointer--;
    registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->program_counter & 0xFF);
    registers->stack_pointer--;

    registers->status &= ~ic_6502_B;
    registers->status |= ic_6502_U;

    registers->mapper->cpu_bus_write(registers->mapper, STACK_BASE + registers->stack_pointer, registers->status);
    registers->stack_pointer--;

    registers->status |= ic_6502_I;
    uint8_t lo = registers->mapper->cpu_bus_read(registers->mapper, IRQ_VECTOR);
    uint8_t hi = registers->mapper->cpu_bus_read(registers->mapper, IRQ_VECTOR + 1);

    registers->program_counter = hi << 8 | lo;
    registers->cycles = 8;
}
