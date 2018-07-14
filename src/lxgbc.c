#include "lxgbc.h"
#include "ram.h"
#include "cpu.h"
#include "mbc.h"
#include "rom.h"
#include "ppu.h"
#include "sound.h"
#include "input.h"
#include "emu.h"

static void handle_event(SDL_Event, gbc_system *);

int main(int argc, char **argv) {

    gbc_system *gbc = malloc(sizeof(gbc_system));

    if(argc != 2) {
        fprintf(stderr, "ERROR: Please specify a rom file\n");
        return EXIT_FAILURE; 
    }

    init_system(gbc, argv[1]);
    init_window(gbc->ppu);

    set_window_title(gbc->ppu->window, gbc->rom->title, false);

    // TEMP: skip logo
    gbc->cpu->registers->PC = 0x100;
    write_byte(gbc, 0xff50, 1, false);

    SDL_Event event;
    unsigned int last_time = 0;

    // Main endless loop 
    while(gbc->is_running) {

        static const unsigned int max_clocks = CLOCK_SPEED / FRAMERATE;
        static const unsigned char millis_per_frame = (1 / FRAMERATE) * 1000.0;

        unsigned int frame_clocks = 0;
        last_time = SDL_GetTicks();

        // Run the clocks for this frame
        while(frame_clocks < max_clocks) {

            unsigned char clocks = 0;

            if(gbc->cpu->is_halted == false) {
                clocks = execute_instr(gbc);                
            } else {
                clocks = 4;
            }

            check_interrupts(gbc);

            update_timer(gbc, clocks);
            update_ppu(gbc, clocks);

            frame_clocks += clocks;
        }

        while(SDL_PollEvent(&event)) {
            handle_event(event, gbc);
        }
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}

// Handles events from SDL
static void handle_event(SDL_Event event, gbc_system *gbc) {

    switch(event.type) {
        case SDL_QUIT:
            gbc->is_running = 0;
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            handle_input(gbc->input, event.key); 
            break;
    }
}
