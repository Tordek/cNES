#include <stdint.h>

#include "SDL2/SDL.h"
#include "controllers.h"

uint8_t controllers_read(struct controllers *controllers, uint16_t address)
{
    if (address == 0) {
        if (controllers->p1_strobe) {
            controllers->p1_latch = controllers->p1_status;
        }

        uint8_t result = controllers->p1_latch & 0x01;

        controllers->p1_latch = (controllers->p1_latch >> 1) | 0x80;

        return result;
    }

    return 0;
}

void controllers_write(struct controllers *controllers, uint16_t address, uint8_t data)
{
    if (address == 0) {
        controllers->p1_strobe = data & 1;
    } else {
        controllers->p2_strobe = data & 1;
    }

    if (controllers->p1_strobe) {
        controllers->p1_latch = controllers->p1_status;
    }
}

int controllers_handle(struct controllers *controllers, SDL_Event *event)
{
    if (event->type != SDL_KEYUP && event->type != SDL_KEYDOWN) {
        return 0; // Not handled
    }

    uint8_t pressmask = 0;
    switch (event->key.keysym.sym) {
        case SDLK_a:
            pressmask = 0x01;
            break;
        case SDLK_s:
            pressmask = 0x02;
            break;
        case SDLK_h:
            pressmask = 0x04;
            break;
        case SDLK_j:
            pressmask = 0x08;
            break;
        case SDLK_UP:
            pressmask = 0x10;
            break;
        case SDLK_DOWN:
            pressmask = 0x20;
            break;
        case SDLK_LEFT:
            pressmask = 0x40;
            break;
        case SDLK_RIGHT:
            pressmask = 0x80;
            break;
    }

    if (event->type == SDL_KEYDOWN) {
        controllers->p1_status |= pressmask;
    } else {
        controllers->p1_status &= ~pressmask;
    }


    return 1;
}
