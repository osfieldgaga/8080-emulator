#include "8080.c"
#include <stdio.h>
#include <stdlib.h>
// #include "SDL2/include/SDL2/SDL.h"
#define SDL_MAIN_HANDLED
#include "SDL2/include/SDL2/SDL.h"
#include "SDL2/include/SDL2/SDL_ttf.h"
#include "constants.h"
#include <time.h>
#include <stdbool.h>

typedef struct ShiftRegister
{
    uint8_t shift_reg_lo; // shift_reg[0]
    uint8_t shift_reg_hi; // shift_reg[1]
    uint8_t shift_offset;

} ShiftRegister;

bool continue_exec = false;

uint8_t r_port[4];
uint8_t w_port[7];

int last_interrupt = 0;
int frame_count = 0;

uint8_t MachineIN(uint8_t port, uint8_t data)
{
    uint8_t acc;
    ShiftRegister *shift;

    switch (port)
    {
    case 1:
    {
        acc = data;
        break;
    }
    case 3:
    {
        // perform the shift register mask with the offset
        uint16_t shift_reg_temp = (shift->shift_reg_hi << 8) | (shift->shift_reg_lo);

        acc = (shift_reg_temp >> (8 - shift->shift_offset)) & 0xff;
        break;
    }

    default:
        break;
    }

    return acc;
}

uint8_t MachineOUT(uint8_t port, uint8_t value)
{

    ShiftRegister *shift;

    switch (port)
    {
    case 2:
    {
        // set offset for the shifting
        shift->shift_offset = value & 0x7;
        break;
    }
    case 4:
    {
        // shift value into shift register
        uint16_t shift_reg_temp = (shift->shift_reg_hi << 8) | (shift->shift_reg_lo);

        // move high bits into low, and value into high
        shift->shift_reg_lo = shift->shift_reg_hi;
        shift->shift_reg_hi = value;

        break;
    }
    default:
        break;
    }
}

uint8_t MachineKeyDown(uint8_t port, uint8_t value)
{
    r_port[1] |= value;
    MachineIN(port, r_port[1]);
}

uint8_t MachineKeyUp(uint8_t port, uint8_t value)
{
    r_port[1] &= value;
    MachineIN(port, r_port[1]);
}

void Interrupt(State8080 *state, uint8_t int_num)
{
    // Basically the implementation of a RST function, which is a special CALL
    // only difference is the value set into the PC
    state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
    state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
    state->sp = (state->sp) - 2;

    state->pc = 8 * int_num;
}

int current_time;
int elapsed_time, max_elapsed;

int main(int argc, char **argv)
{

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("Couldn't init screen and keyboard (SDL): %s\n", SDL_GetError());
        return 0;
    }
    printf("Screen and keyboard initialized (SDL)");

    if (TTF_Init() < 0)
    {
        printf("Error intializing SDL_ttf: %s\n", SDL_GetError());
        return 0;
    }

    SDL_Window *window = SDL_CreateWindow("Space Invaders", (1920/2) - 256, SDL_WINDOWPOS_CENTERED, 256, 300, 0);
    if (!window)
    {
        printf("Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    // SDL_Window *window2 = SDL_CreateWindow("Machine State", (1920/2) + 256, SDL_WINDOWPOS_CENTERED, 512, 720, 0);
    // if (!window2)
    // {
    //     printf("Error creating window: %s\n", SDL_GetError());
    //     SDL_Quit();
    //     return 0;
    // }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    TTF_Font *font;

    font = TTF_OpenFont("assets/arial.ttf", 12);
    if (!font)
    {
        printf("Failed to load font: %s\n", TTF_GetError());
    }

    SDL_Event event;

    // first, load the code into memory
    State8080 *state = (State8080 *)malloc(sizeof(State8080));

    InitializeRegisters(state);
    InitializeMemory(state);

    int opbytes = 1;
    int instr_count = 0;

    if (!state)
    {
        printf("error: Unable to allocate memory for state.\n");
        exit(1);
    }

    if (!state->memory)
    {
        printf("error: Unable to allocate memory for 8080 memory.\n");
        exit(1);
    }

    FILE *fp = NULL;

    if (!FOR_CPUDIAG)
    {
        fp = fopen("invaders.rom", "rb");
    }
    else
    {
        fp = fopen("cpudiag_offset.bin", "rb");
    }

    if (fp == NULL)
    {
        printf("error: Couldn't open %s\n", argv[1]);
        exit(1);
    }

    // set the pointer at the end of the file to get the size
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    printf("%d", fsize);

    // put the cursor back at the beginning
    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = (unsigned char *)malloc(fsize);

    fread(buffer, fsize, 1, fp);
    fclose(fp);

    // state->memory = buffer;

    // move the content of the rom to memory
    int i = 0;
    // printf("Memory map: %02x \n", fsize);
    for (i; i <= fsize; i++)
    {
        state->memory[i] = (uint8_t)buffer[i];
        // printf("%02x   %02x [%02x]\n", i, (state->memory[i]), buffer[i]);
        // printf("%02x   %02x\n", i, buffer[i]);
    }

    state->pc = 0;

    if (FOR_CPUDIAG)
    {
        // Fix the first instruction to be JMP 0x100
        state->memory[0] = 0xc3;
        state->memory[1] = 0x00;
        state->memory[2] = 0x01;
        // Fix the stack pointer from 0x6ad to 0x7ad
        //  this 0x06 byte 112 in the code, which is
        //  byte 112 + 0x100 = 368 in memory
        state->memory[368] = 0x7;

        // Skip DAA test
        // state->memory[0x59c] = 0xc3; // JMP
        // state->memory[0x59d] = 0xc2;
        // state->memory[0x59e] = 0x05;
    }

    // let the engine run

    // -------------------------------------------------------

    SDL_Surface *text_l;
    SDL_Surface *text_r;
    SDL_Surface *text_shoot;

    uint8_t r = 0;

    if (r_port[1] >> 5 & 0x01)
    {
        r = 255;
    }

    SDL_Color color_l = {r, 0, 0};

    text_l = TTF_RenderText_Solid(font, "Left", color_l);
    SDL_Texture *text_texture_l;
    text_texture_l = SDL_CreateTextureFromSurface(renderer, text_l);

    r = 0;

    if (r_port[1] >> 6 & 0x01)
    {
        r = 255;
    }
    SDL_Color color_r = {r, 0, 0};

    text_r = TTF_RenderText_Solid(font, "Right", color_r);
    SDL_Texture *text_texture_r;
    text_texture_r = SDL_CreateTextureFromSurface(renderer, text_r);

    r = 0;

    if (r_port[1] >> 4 & 0x01)
    {
        r = 255;
    }
    SDL_Color color_shoot = {r, 0, 0};

    text_shoot = TTF_RenderText_Solid(font, "Shoot", color_shoot);
    SDL_Texture *text_texture_shoot;
    text_texture_shoot = SDL_CreateTextureFromSurface(renderer, text_shoot);

    SDL_Rect states = {
        0, 244, 256, 56};

    // -------------------------------------------------------

    int last_frame_time = SDL_GetTicks();

    while (1)
    {
        // system("@cls||clear");

        current_time = SDL_GetTicks();
        elapsed_time = (current_time - last_frame_time);

        // printf("Current time (ms): %d\n", current_time);
        // printf("Elapsed time (ms): %d\n", elapsed_time);
        // printf("Last frame time (ms): %d\n", last_frame_time);
        // printf("Max elapsed time (ms): %d\n", max_elapsed);

        uint8_t *code = &state->memory[state->pc];

        int emul_time_start = SDL_GetTicks();

        if (*code == 0xdb)
        {
            // implement IN
            // printf("Running IN %02x\n", code[1]);
            uint8_t port = code[1]; // the port is in the following byte

            state->a = MachineIN(port, state->a); //
            state->pc+=2;
            state->cycles += 3;
        }
        else if (*code == 0xd3)
        {
            // implement out
            // printf("Running OUT %02x\n", code[1]);
            uint8_t port = code[1]; // the data is in the following byte

            MachineOUT(port, state->a); // the value to be sent is stored in the accumulator before calling OUT
            state->pc+=2;
            state->cycles += 3;

        }
        else
        {
            opbytes = Emulate8080(state);
            state->pc += opbytes;
            instr_count++;
            if (LOGS_CPU)
            {
                // system("@cls||clear");
                printf("Instructions ran: %d\n", instr_count);
                printf("Prev instructions: %02x\n", state->memory[state->pc - opbytes]);
                printf("Current instructions: %02x\n", state->memory[state->pc]);
                ShowState(state);
            }
            // system("@cls||clear");
            // printf("Cycles: %d\n", state->cycles);
            printf("%d\n", instr_count);
            opbytes = 1;

            // if (instr_count == 40000) exit(1);
            // if (state->pc == 0xada) exit(1);

        }

        // printf("CPU time: %d\n", SDL_GetTicks() - emul_time_start);

        if (SDL_PollEvent(&event))
        {
            if (SDL_QUIT == event.type)
            {
                break;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_SPACE:
                    MachineKeyDown(1, 0x10);
                    if (MANUAL_EXEC)
                    {
                        state->pc += opbytes;
                        continue_exec = true;
                    }
                    break;
                case SDL_SCANCODE_LEFT:
                    MachineKeyDown(1, 0x20);
                    break;
                case SDL_SCANCODE_RIGHT:
                    MachineKeyDown(1, 0x40);
                    break;
                }
            }

            if (event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_SPACE:
                    MachineKeyUp(1, 0xEF); // 0xBF = 0b11101111 which will set only bit 4 off
                    break;
                case SDL_SCANCODE_LEFT:
                    MachineKeyUp(1, 0xDF); // 0xDF = 0b11011111 which will set only bit 5 off
                    break;
                case SDL_SCANCODE_RIGHT:
                    MachineKeyUp(1, 0xBF); // 0xBF = 0b10111111 which will set only bit 6 off
                    break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &states);

        // Get the current time
        current_time = clock();

        // Calculate the elapsed time in seconds
        // elapsed_time = ((double)(current_time - start_time)) / CLOCKS_PER_SEC;

        int i, j, k;
        for (i = 0; i < 256; i++)
        {
            for (j = 0; j < 28; j++)
            {
                int offset = (i * 28) + j;
                // printf("(%d, %d): %02x\n", i, j, state->memory[0x2400 + offset]);
                uint8_t render_pixels;
                if (FOR_CPUDIAG)
                {
                    render_pixels = state->memory[offset];
                }
                else
                {
                    render_pixels = state->memory[0x2400 + offset];
                }

                for (k = 0; k < 8; k++)
                {
                    uint8_t pixel_state = (render_pixels >> k) & 0x01;

                    if (pixel_state)
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    }

                    SDL_RenderDrawPoint(renderer, i, j);
                }
            }
        }

        frame_count++;

        // printf("Elapsed time (ms): %d\n", elapsed_time);
        if (elapsed_time >= 8.33 && elapsed_time < 16.67)
        {
            // Generate the VBlank interrupt or perform the action here
            if (state->int_enabled)
            {
                Interrupt(state, 1);
                opbytes = 1;
                printf("Mid screen interrupt generated\n");
            }
        }

        if (elapsed_time >= 16.67)
        {
            // Generate the VBlank interrupt or perform the action here

            // Reset the start time to the current time for the next loop
            last_frame_time = current_time;

            if (elapsed_time > max_elapsed)
            {
                max_elapsed = elapsed_time;
            }

            if (state->int_enabled)
            {
                Interrupt(state, 2);
                opbytes = 1;
                printf("VBL interrupt generated\n");
            }
        }

        SDL_RenderCopy(renderer, text_texture_l, NULL, &(SDL_Rect){0, 244, text_l->w, text_l->h});
        SDL_RenderCopy(renderer, text_texture_r, NULL, &(SDL_Rect){text_l->w + 10, 244, text_r->w, text_r->h});
        SDL_RenderCopy(renderer, text_texture_shoot, NULL, &(SDL_Rect){0, 244 + text_l->h + 5, text_shoot->w, text_shoot->h});

        SDL_RenderPresent(renderer);

        // printf("Render time: %d\n", SDL_GetTicks() - emul_time_start);

        if (LOGS_MACHINE)
        {
            system("@cls||clear");

            printf("Port 1 %02x\n", r_port[1]);
            printf("Time %.4f\n", elapsed_time);
            printf("Max elapsed %.4f\n", max_elapsed);
            printf("Frames displayed %d\n", frame_count);
        }
    }

    SDL_DestroyTexture(text_texture_l);
    SDL_DestroyTexture(text_texture_r);
    SDL_DestroyTexture(text_texture_shoot);

    SDL_FreeSurface(text_l);
    SDL_FreeSurface(text_r);
    SDL_FreeSurface(text_shoot);

    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}