#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <SDL.h>

typedef struct {
    SDL_Window* window;
    SDL_Surface* screen;
} Game;

unsigned char fontset[80] = 
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xf0, 0x10, 0xf0,
    0xF0, 0x80, 0xf0, 0x90, 0xf0,
    0xf0, 0x10, 0x20, 0x40, 0x40,
    0xf0, 0x90, 0xF0, 0x90, 0xF0,
    0xf0, 0x90, 0xf0, 0x10, 0xf0,
    0xf0, 0x90, 0xf0, 0x90, 0x90,
    0xe0, 0x90, 0xe0, 0x90, 0xe0,
    0xf0, 0x90, 0x80, 0x80, 0xf0,
    0xe0, 0x90, 0x90, 0x90, 0xe0,
    0xF0, 0x80, 0xf0, 0x80, 0xf0,
    0xf0, 0x80, 0xf0, 0x80, 0x80
};

static SDL_Scancode keyMappings[16] = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V,
};

typedef struct {
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16];
    unsigned short indexRegister;
    unsigned short programCounter;
    unsigned char graphics[64][32];
    unsigned char delayTimer;
    unsigned char soundTimer;
    unsigned short stack[16];
    unsigned short stackPointer;
    uint8_t keys[16];
    uint8_t draw;
    int fileDescriptor;
} Cpu;

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("No file specified");
        exit(1);
    }

    Game game = {};

    printf("Initialising SDL2");

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "SDL2 loading failure: %s\n", SDL_GetError());
        return 1;
    }

    game.window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, 0);

    if(!game.window)
    {
        fprintf(stderr, "SDL2 window loading failure: %s\n", SDL_GetError());
        return 1;
    }

    game.screen = SDL_GetWindowSurface(game.window);

    Cpu cpu = {};
    cpu.programCounter = 0x200;
    cpu.opcode = 0;
    cpu.indexRegister = 0;
    cpu.stackPointer = 0;

    printf("Opening ROM\n"); 
    cpu.fileDescriptor = open(argv[1], O_RDONLY);

    if(cpu.fileDescriptor < 0) 
    {
        printf("Could not open ROM: %s", argv[1]);
        exit(1);
    }

    for(int i = 0; i < 16; i++)
    {
        cpu.V[i] = 0;
    }

    for(int i = 0; i < 16; i++)
    {
        cpu.stack[i] = 0;
    }

    for(int i = 0; i < 16; i++)
    {
        cpu.keys[i] = 0;
    }


    for(int i = 0; i < 4096; i++)
    {
        cpu.memory[i] = 0;
    }

    for(int i = 0; i < 80; i++)
    {
        cpu.memory[i] = fontset[i];
    }

    int bufferSize = 512;
    int bufferReadSize = 0;
    int i = 0;
    int j = 0;
    char buffer[bufferSize];

    while((bufferReadSize = read(cpu.fileDescriptor, &buffer, bufferSize)) > 0)
    {
        for(j = 0; j < bufferReadSize; j++)
        {
            cpu.memory[512 + i] = buffer[i];
            i++;
        }
    }

    if(bufferReadSize < 0)
    {
        printf("Error reading file");
        return 1;
    }

    close(cpu.fileDescriptor);

    SDL_Event event;
    for(;;)
    {
        while(SDL_PollEvent(&event) != 0)
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    return 1;
                    break;
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);

        for(int i = 0; i < 16; i++)
        {
            cpu.keys[i] = state[keyMappings[i]];
        }

        int opfound = 0;
        cpu.opcode = cpu.memory[cpu.programCounter] << 8 | cpu.memory[cpu.programCounter + 1];
        unsigned short int height = 0;
        unsigned short int x = 0;
        unsigned short int y = 0;

        switch(cpu.opcode & 0xF000)
        {
            case 0x1000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.opcode & 0x0FFF;
                opfound = 1;
                break;
            case 0x2000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.stack[cpu.stackPointer] = cpu.programCounter + 2;
                cpu.stackPointer++;
                cpu.programCounter = cpu.opcode & 0x0FFF;
                opfound = 1;
                break;
            case 0x3000:
                if(cpu.V[(cpu.opcode & 0x0F00) >> 8] == (cpu.opcode & 0x00FF))
                {
                    cpu.programCounter += 4;
                }
                else
                {
                    cpu.programCounter += 2;
                }

                printf("cpu opcode found: %x\n", cpu.opcode);
                opfound = 1;
                break;
            case 0x4000:
                if(cpu.V[(cpu.opcode & 0x0F00) >> 8] != (cpu.opcode & 0x00FF))
                {
                    cpu.programCounter += 4;
                }
                else
                {
                    cpu.programCounter += 2;
                }
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0x6000:
                cpu.V[(cpu.opcode & 0xF00) >> 8] = cpu.opcode & 0x00FF;
                printf("cpu opcode found: %x\n", cpu.opcode);
                printf("V[%x] = %x\n", (cpu.opcode & 0x0F00) >> 8, cpu.opcode & 0x00FF);
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0x7000:
                cpu.V[(cpu.opcode & 0xF00) >> 8] += cpu.opcode & 0x00FF;
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xA000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.indexRegister = cpu.opcode & 0x0FFF;
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xB000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = (cpu.opcode & 0x0FFF) + cpu.V[0];
                opfound = 1;
                break;
            case 0xC000: 
                cpu.V[(cpu.opcode & 0x0F00) >> 8] = (rand() % 256) & 0x00FF;
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xD000:
                cpu.V[0xF] = 0;
                height = cpu.opcode & 0x000F;
                x = cpu.V[(cpu.opcode & 0x0F00) >> 8];
                y = cpu.V[(cpu.opcode & 0x00F0) >> 4];

                for(int i = 0; i < height; i++)
                {
                    for(int j = 0; j < 8; j++)
                    {
                        if((cpu.memory[cpu.indexRegister + i] & (0x80 >> j)) == 0)
                        {
                            continue;
                        }

                        if(cpu.graphics[x + j][y + i] == 1)
                        {
                            cpu.V[0xF] = 1;
                        }

                        cpu.graphics[x + j][y + i] ^= 1;
                    }
                    
                }
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter += 2;
                cpu.draw = 1;
                opfound = 1;
                break;
        }

        switch(cpu.opcode & 0xF0FF)
        {
            case 0xF00A: 
                printf("cpu opcode found: %x\n", cpu.opcode);

                for(int i = 0; i < 16; i++)
                {
                    if(cpu.keys[i] == 1)
                    {
                        cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.keys[i];
                        cpu.programCounter += 2;
                    }
                }

                opfound = 1;
                break;
            case 0xF015: 
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter += 2;
                cpu.delayTimer = cpu.V[(cpu.opcode & 0x0F00) >> 8];
                opfound = 1;
                break;
            case 0xF01E:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.indexRegister += cpu.V[(cpu.opcode & 0x0F00) >> 8];
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xF007: 
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.delayTimer;
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xF029:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.indexRegister = cpu.V[(cpu.opcode & 0x0F00 >> 8)] * 5;
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xF055:
                printf("cpu opcode found: %x\n", cpu.opcode);
                for(int i = 0; i < (cpu.opcode & 0x0F00) >> 8; i++)
                {
                  cpu.memory[cpu.indexRegister + i] = cpu.V[(cpu.opcode & 0x0F00) >> 8];
                }

                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xF033:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.memory[cpu.indexRegister] = cpu.V[(cpu.opcode & 0x0F00) >> 8] / 100;
                cpu.memory[cpu.indexRegister + 1] = (cpu.V[(cpu.opcode & 0x0F00) >> 8] / 10) % 10;
                cpu.memory[cpu.indexRegister + 2] = (cpu.V[(cpu.opcode & 0x0F00) >> 8] / 10) % 1;

                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xF065:
                printf("cpu opcode found: %x\n", cpu.opcode);

                for(int i = 0; i < (cpu.opcode & 0x0F00) >> 8; i++)
                {
                    cpu.V[i] = cpu.memory[cpu.indexRegister + i];
                }

                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0xE09E:
                printf("cpu opcode found: %x\n", cpu.opcode);

                if(cpu.keys[(cpu.opcode & 0x0F00) >> 8] == 1)
                {
                    cpu.programCounter += 4;
                }
                else
                {
                    cpu.programCounter += 2;
                }

                opfound = 1;
                break;
            case 0xE0A1:
                printf("cpu opcode found: %x\n", cpu.opcode);
                if(cpu.keys[(cpu.opcode & 0x0F00) >> 8] == 0)
                {
                    cpu.programCounter += 2;
                }
                else
                {
                    cpu.programCounter += 4;
                }

                opfound = 1;
                break;
        }

        switch(cpu.opcode & 0x00FF)
        {
            case 0x00E0:
                
                for(int i = 0; i < 64; i++)
                {
                    for(int j = 0; j < 32; j++)
                    {
                        cpu.graphics[i][j] = 0x0;
                    }

                }

                cpu.programCounter += 2;
                cpu.draw = 1;
                opfound = 1;
                break;
            case 0x00EE:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.stackPointer--;
                cpu.programCounter = cpu.stack[cpu.stackPointer];
                cpu.programCounter += 2;

                opfound = 1;
                break;
        }
        
        switch(cpu.opcode & 0xF00F)
        {
            case 0x8000:
                cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.V[(cpu.opcode & 0x00F0) >> 4];
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter += 2;
                opfound = 1;
                break;
            case 0x9000:
                if(cpu.V[(cpu.opcode & 0x0F00) >> 8] != cpu.V[(cpu.opcode & 0x00F0) >> 4])
                {
                    cpu.programCounter += 4;
                }
                else
                {
                    cpu.programCounter += 2;
                }
                opfound = 1;
                break;

        }
        
        if(opfound == 0)
        {
            printf("opcode not found: %x\n", cpu.opcode);
            break;
        }

        if(cpu.soundTimer < 1)
        {
            cpu.soundTimer--;
        }

        if(cpu.delayTimer < 1)
        {
            cpu.delayTimer--;
        }


        if(cpu.draw == 1)
        {

            if(SDL_MUSTLOCK(game.screen))
            {
                SDL_LockSurface(game.screen);
            }

            Uint8 colour;
            SDL_Rect pixel = {0, 0, 10, 10};
            for(int i = 0; i < 64; i++)
            {
                pixel.x = i * 10;
                for(int j = 0; j < 32; j++)
                {
                    pixel.y = j * 10;
                    if(cpu.graphics[i][j] == 1)
                    {
                        colour = 255;
                    }
                    else
                    {
                        colour = 0;
                    }

                    SDL_FillRect(game.screen, &pixel, SDL_MapRGB(game.screen->format, colour, colour, colour));
                }

            }

            if(SDL_MUSTLOCK(game.screen))
            {
                SDL_UnlockSurface(game.screen);
            }

        }
        SDL_UpdateWindowSurface(game.window);
    }

    SDL_DestroyWindow(game.window);
    SDL_Quit();

    return 0;
}
