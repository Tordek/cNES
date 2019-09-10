#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "nes_header.h"

int read_rom(struct nes_rom *rom, char *filename)
{
    FILE *rom_file = fopen(filename, "rb");

    if (rom_file == NULL) {
        return -1;
    }

    uint8_t header_data[16];
    int rc = fread(header_data, 16, 1, rom_file);
    if (rc != 1) {
        return -3;
    }

    if (!(header_data[0] == 'N' && header_data[1] == 'E' && header_data[2] == 'S' && header_data[3] == 0x1A)) {
        return -2;
    }

    rom->prg_rom_size = header_data[4];
    rom->chr_rom_size = header_data[5];

    rom->mirroring = header_data[6] & 0x01;
    rom->non_volatile_memory = header_data[6] & 0x02;
    rom->trainer = header_data[6] & 0x04;
    rom->four_screen_mode = header_data[6] & 0x08;

    rom->mapper_id = header_data[6] >> 4;
    rom->mapper_id |= header_data[6] & 0xF0;
    rom->mapper_id |= (header_data[8] & 0x0F) << 8;
    rom->submapper_id = header_data[8] >> 4;

    rom->prg_rom_size |= (header_data[9] & 0x0F) << 8;
    rom->chr_rom_size |= (header_data[9] & 0xF0) << 4;

    rom->prg_rom = malloc(0x4000 * rom->prg_rom_size);
    fread(rom->prg_rom, 0x4000, rom->prg_rom_size, rom_file);

    if (rom->chr_rom_size > 0) {
        rom->chr_rom = malloc(0x2000 * rom->chr_rom_size);
        fread(rom->chr_rom, 0x2000, rom->chr_rom_size, rom_file);
    }

    fclose(rom_file);
    return 0;
}

void close_rom(struct nes_rom *rom)
{
    if (rom->prg_rom != NULL) {
        free(rom->prg_rom);
    }

    if (rom->chr_rom != NULL) {
        free(rom->chr_rom);
    }
}
