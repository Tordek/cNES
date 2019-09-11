#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>

#include "SDL2/SDL.h"
#include "apu.h"
#include "ic_6502.h"
#include "ic_2c02.h"
#include "nes_header.h"
#include "mapper.h"

int const is_illegal[256] = {
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
};

char const names[256][4] = {
    "BRK", "ORA", "KIL", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
    "BPL", "ORA", "KIL", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
    "JSR", "AND", "KIL", "RLA", "BIT", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
    "BMI", "AND", "KIL", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
    "RTI", "EOR", "KIL", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ALR", "JMP", "EOR", "LSR", "SRE",
    "BVC", "EOR", "KIL", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
    "RTS", "ADC", "KIL", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
    "BVS", "ADC", "KIL", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",
    "NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "XAA", "STY", "STA", "STX", "SAX",
    "BCC", "STA", "KIL", "AHX", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "TAS", "SHY", "STA", "SHX", "AHX",
    "LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LAX", "LDY", "LDA", "LDX", "LAX",
    "BCS", "LDA", "KIL", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
    "CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "AXS", "CPY", "CMP", "DEC", "DCP",
    "BNE", "CMP", "KIL", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
    "CPX", "SBC", "NOP", "ISB", "CPX", "SBC", "INC", "ISB", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISB",
    "BEQ", "SBC", "KIL", "ISB", "NOP", "SBC", "INC", "ISB", "SED", "SBC", "NOP", "ISB", "NOP", "SBC", "INC", "ISB",
};

void print_status(int32_t total_cycles, struct ic_6502_registers *cpu, struct ic_2C02_registers *ppu, struct mapper *mapper);

void print_status(int32_t total_cycles, struct ic_6502_registers *cpu, struct ic_2C02_registers *ppu, struct mapper *mapper)
{
    printf("%04X ", cpu->program_counter);
    uint8_t opcode = mapper->cpu_bus_read(mapper, cpu->program_counter);

    switch(opcode) {
        case 0x00: case 0x08: case 0x18: case 0x1a: case 0x28: case 0x38: case 0x3a: case 0x40:
        case 0x48: case 0x58: case 0x5a: case 0x60: case 0x68: case 0x78: case 0x7a: case 0x82:
        case 0x88: case 0x89: case 0x8a: case 0x98: case 0x9a: case 0xa8: case 0xaa: case 0xb8:
        case 0xba: case 0xc2: case 0xc8: case 0xca: case 0xd8: case 0xda: case 0xe2: case 0xe8:
        case 0xea: case 0xf8: case 0xfa:
        {
            // IMP
            printf(" %02X       ", opcode);
            printf("%c%s                             ", is_illegal[opcode] ? '*' : ' ', names[opcode]);
        }
            break;

        case 0x0a: case 0x2a: case 0x4a: case 0x6a:
        {
            // ACC
            printf(" %02X       ", opcode);
            printf("%c%s A                           ", is_illegal[opcode] ? '*' : ' ', names[opcode]);
        }
        break;

        case 0x02: case 0x09: case 0x0b: case 0x12: case 0x22: case 0x29: case 0x2b: case 0x32:
        case 0x42: case 0x49: case 0x4b: case 0x52: case 0x62: case 0x69: case 0x6b: case 0x72:
        case 0x80: case 0x8b: case 0x92: case 0xa0: case 0xa2: case 0xa9: case 0xab: case 0xb2:
        case 0xc0: case 0xc9: case 0xcb: case 0xd2: case 0xe0: case 0xe9: case 0xeb: case 0xf2:
            {
                // IMM
                uint8_t ad = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, ad);

                printf("%c%s #$%02X                        ", is_illegal[opcode] ? '*' : ' ', names[opcode], ad);
            }
            break;

        case 0x04: case 0x05: case 0x06: case 0x07: case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x44: case 0x45: case 0x46: case 0x47: case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x84: case 0x85: case 0x86: case 0x87: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xc4: case 0xc5: case 0xc6: case 0xc7: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
            {
                // ZP0
                uint8_t ad = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, ad);

                uint8_t val = mapper->cpu_bus_read(mapper, ad);
                printf("%c%s $%02X = %02X                    ", is_illegal[opcode] ? '*' : ' ', names[opcode], ad, val);
            }
            break;

        case 0x14: case 0x15: case 0x16: case 0x17: case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x54: case 0x55: case 0x56: case 0x57: case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x94: case 0x95: case 0xb4: case 0xb5: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
        case 0xf4: case 0xf5: case 0xf6: case 0xf7:
            {
                // ZPX
                uint8_t ad = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, ad);

                uint8_t data = mapper->cpu_bus_read(mapper, ad + cpu->x & 0xFF);
                printf("%c%s $%02X,X @ %02X = %02X             ", is_illegal[opcode] ? '*' : ' ', names[opcode], ad, ad + cpu->x & 0xFF, data);
            }
            break;

        case 0x96: case 0x97: case 0xb6: case 0xb7:
            {
                // ZPY
                uint8_t ad = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, ad);

                uint8_t data = mapper->cpu_bus_read(mapper, ad + cpu->y & 0xFF);
                printf("%c%s $%02X,Y @ %02X = %02X             ", is_illegal[opcode] ? '*' : ' ', names[opcode], ad, ad + cpu->y & 0xFF, data);
            }
            break;

        case 0x10: case 0x30: case 0x50: case 0x70: case 0x90: case 0xb0: case 0xd0: case 0xf0:
            {
                // REL
                int8_t addr_rel = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, addr_rel & 0xFF);
                printf("%c%s $%04X                       ", is_illegal[opcode] ? '*' : ' ', names[opcode], cpu->program_counter + addr_rel + 2);
            }
            break;

        case 0x20: case 0x4c:
            {
                // ABS
                uint16_t lo = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                uint16_t hi = mapper->cpu_bus_read(mapper, cpu->program_counter + 2);
                printf(" %02X %02X %02X ", opcode, lo, hi);

                printf("%c%s $%04X                       ", is_illegal[opcode] ? '*' : ' ', names[opcode],  hi << 8 | lo);
            }
            break;

        case 0x0c: case 0x0d: case 0x0e: case 0x0f: case 0x2c: case 0x2d: case 0x2e: case 0x2f: case 0x4d: case 0x6d:
        case 0x6e: case 0x6f: case 0x8d: case 0x8c: case 0x8e: case 0x8f: case 0xac: case 0xad: case 0x4e: case 0x4f: case 0xae:
        case 0xaf: case 0xcc: case 0xcd: case 0xce: case 0xcf: case 0xec: case 0xed: case 0xee:
        case 0xef:
            {
                // ABS (not jumps)
                uint16_t lo = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                uint16_t hi = mapper->cpu_bus_read(mapper, cpu->program_counter + 2);
                printf(" %02X %02X %02X ", opcode, lo, hi);

                uint8_t val = mapper->cpu_bus_read(mapper, hi << 8 | lo);

                printf("%c%s $%04X = %02X                  ", is_illegal[opcode] ? '*' : ' ', names[opcode],  hi << 8 | lo, val);
            }
            break;

        case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        case 0x5c: case 0x5d: case 0x5e: case 0x5f: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
        case 0x9c: case 0x9d: case 0xbc: case 0xbd: case 0xdc: case 0xdd: case 0xde: case 0xdf:
        case 0xfc: case 0xfd: case 0xfe: case 0xff:
            {
                // ABX
                uint8_t lo = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                uint8_t hi = mapper->cpu_bus_read(mapper, cpu->program_counter + 2);
                printf(" %02X %02X %02X ", opcode, lo, hi);

                uint16_t base = hi << 8 | lo;
                uint16_t addr = base + cpu->x;
                uint8_t data = mapper->cpu_bus_read(mapper, addr);

                printf("%c%s $%04X,X @ %04X = %02X         ", is_illegal[opcode] ? '*' : ' ', names[opcode], base, addr, data);
            }
            break;

        case 0x19: case 0x1b: case 0x39: case 0x3b: case 0x59: case 0x5b: case 0x79: case 0x7b:
        case 0x99: case 0x9b: case 0x9e: case 0x9f: case 0xb9: case 0xbb: case 0xbe: case 0xbf:
        case 0xd9: case 0xdb: case 0xf9: case 0xfb:
            {
                // ABY
                uint8_t lo = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                uint8_t hi = mapper->cpu_bus_read(mapper, cpu->program_counter + 2);
                printf(" %02X %02X %02X ", opcode, lo, hi);

                uint16_t base = hi << 8 | lo;
                uint16_t addr = base + cpu->y;

                uint8_t data = mapper->cpu_bus_read(mapper, addr);

                printf("%c%s $%04X,Y @ %04X = %02X         ", is_illegal[opcode] ? '*' : ' ', names[opcode], base, addr, data);
            }
            break;

        case 0x6c:
            {
                // IND (Handles page boundary bug)
                uint8_t lo = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                uint8_t hi = mapper->cpu_bus_read(mapper, cpu->program_counter + 2);
                printf(" %02X %02X %02X ", opcode, lo, hi);

                uint16_t addr = hi << 8 | lo;

                uint16_t val = mapper->cpu_bus_read(mapper, hi << 8 | lo);
                lo++;
                val |= mapper->cpu_bus_read(mapper, hi << 8 | lo) << 8;

                printf("%c%s ($%04X) = %04X              ", is_illegal[opcode] ? '*' : ' ', names[opcode],  addr, val);
            }

            break;

        case 0x01: case 0x03: case 0x21: case 0x23: case 0x41: case 0x43: case 0x61: case 0x63:
        case 0x81: case 0x83: case 0xa1: case 0xa3: case 0xc1: case 0xc3: case 0xe1: case 0xe3:
            {
                // IZX
                uint8_t ad = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, ad);

                uint8_t base = ad + cpu->x;
                uint16_t addr = mapper->cpu_bus_read(mapper, base);
                addr |= mapper->cpu_bus_read(mapper, (base + 1) & 0xFF) << 8;

                uint8_t val = mapper->cpu_bus_read(mapper, addr);
                printf("%c%s ($%02X,X) @ %02X = %04X = %02X    ", is_illegal[opcode] ? '*' : ' ', names[opcode], ad, base, addr, val);
            }

            break;

        case 0x11: case 0x13: case 0x31: case 0x33: case 0x51: case 0x53: case 0x71: case 0x73:
        case 0x91: case 0x93: case 0xb1: case 0xb3: case 0xd1: case 0xd3: case 0xf1: case 0xf3:
            {
                // IZY
                uint8_t ad = mapper->cpu_bus_read(mapper, cpu->program_counter + 1);
                printf(" %02X %02X    ", opcode, ad);

                uint16_t base = mapper->cpu_bus_read(mapper, ad);
                base |= mapper->cpu_bus_read(mapper, (ad + 1) & 0xFF) << 8;

                uint16_t addr = base + cpu->y;

                uint8_t val = mapper->cpu_bus_read(mapper, addr);
                printf("%c%s ($%02X),Y = %04X @ %04X = %02X  ", is_illegal[opcode] ? '*' : ' ', names[opcode], ad, base, addr, val);
            }

            break;
    }

    //codes, dis = cpu.disassemble(cpu.pc);
    printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X ", cpu->a, cpu->x, cpu->y, cpu->status, cpu->stack_pointer);
    // "PPU:%3d,%3d CYC:%d\n", , 0, 0, 0 /*cpu->total_cycles - 1*/
    printf("PPU:%3d,%3d CYC:%d", ppu->clock % 341, ppu->clock / 341, total_cycles);
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s rom\n", argv[0]);
        return -1;
    }

    struct nes_rom rom;
    int rc = read_rom(&rom, argv[1]);

    if (rc) {
        printf("Not a iNES/NES2.0 file");
        return rc;
    }

    printf("ROM info: %s\n", argv[1]);
    printf("Mapper: %d\n", rom.mapper_id);

    struct ic_6502_registers cpu;
    struct ic_2C02_registers ppu;
    struct apu apu;

    struct mapper *mapper = (mappers[rom.mapper_id])(&rom);
    mapper->cpu = &cpu;
    mapper->ppu = &ppu;
    mapper->apu = &apu;
    cpu.mapper = mapper;
    ppu.mapper = mapper;

    ic_2c02_reset(&ppu);
    ic_6502_reset(&cpu);

    ppu.clock=-24;

    cpu.program_counter = 0xC000;

    uint32_t total_cycles = 0;
    int count = 8992;
    while (count--) {
        do {
            ppu.clock+=3;
            ic_6502_clock(&cpu);
            total_cycles += 1;
        } while(cpu.cycles > 0);
        print_status(total_cycles - 1, &cpu, &ppu, mapper);
    }

    return 0;
}
