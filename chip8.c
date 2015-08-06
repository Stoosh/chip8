#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>

typedef struct {
    SDL_Window* window;
    SDL_Surface* screen;
} Game;

typedef struct {
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16];
    unsigned short indexRegister;
    unsigned short programCounter;
    unsigned char graphics[64][32];
    unsigned char delayTimer;
    unsigned short stack[16];
    unsigned short stackPointer;
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
    cpu.programCounter = 512;
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

        int opfound = 0;
        cpu.opcode = cpu.memory[cpu.programCounter] << 8 | cpu.memory[cpu.programCounter + 1];
        unsigned short int height = 0;
        unsigned short int x = 0;
        unsigned short int y = 0;
        unsigned short int pixel = 0;

        switch(cpu.opcode & 0xF000)
        {
            /* case 0x0000: */
            /*     printf("cpu opcode found: %x\n", cpu.opcode); */
            /*     printf("pc: %x\n", cpu.opcode & 0x0FFF); */
            /*     cpu.programCounter = cpu.opcode & 0x0FFF; */
            /*     opfound = 1; */
            /*     break; */
            case 0x1000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.opcode & 0x0FFF;
                opfound = 1;
                break;
            case 0x2000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.stack[cpu.stackPointer] = cpu.programCounter;
                cpu.stackPointer++;
                cpu.programCounter = cpu.opcode & 0x0FFF;
                opfound = 1;
                break;
            case 0x3000:
                if(cpu.V[(cpu.opcode & 0x0F00) >> 8] == (cpu.opcode & 0x00FF))
                {
                    cpu.programCounter = cpu.programCounter + 4;
                }
                else
                {
                    cpu.programCounter = cpu.programCounter + 2;
                }

                printf("cpu opcode found: %x\n", cpu.opcode);
                opfound = 1;
                break;
            case 0x4000:
                if(cpu.V[(cpu.opcode & 0x0F00) >> 8] != (cpu.opcode & 0x00FF))
                {
                    cpu.programCounter = cpu.programCounter + 4;
                }
                else
                {
                    cpu.programCounter = cpu.programCounter + 2;
                }
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0x6000:
                cpu.V[(cpu.opcode & 0xF00) >> 8] = cpu.opcode & 0x00FF;
                printf("cpu opcode found: %x\n", cpu.opcode);
                printf("V[%x] = %x\n", (cpu.opcode & 0x0F00) >> 8, cpu.opcode & 0x00FF);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0x7000:
                cpu.V[(cpu.opcode & 0xF00) >> 8] += cpu.opcode & 0x00FF;
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xA000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.indexRegister = cpu.opcode & 0x0FFF;
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xB000:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = (cpu.opcode & 0x0FFF) + cpu.V[0];
                opfound = 1;
                break;
            case 0xC000: 
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xD000:
                cpu.V[0xF] = 0;
                height = cpu.opcode & 0x000F;
                x = cpu.V[(cpu.opcode & 0x0F00) >> 8];
                y = cpu.V[(cpu.opcode & 0x00F0) >> 4];

                for(int i = 0; i < height; i++)
                {
                    pixel = cpu.memory[cpu.indexRegister + i];

                    for(int j = 0; j < 8; j++)
                    {
                        if((pixel & (0x80 >> j)) == 0)
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
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
        }

        switch(cpu.opcode & 0xF0FF)
        {
            case 0xF015: 
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                cpu.delayTimer = cpu.V[(cpu.opcode & 0x0F00) >> 8];
                opfound = 1;
                break;
            case 0xF007: 
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.delayTimer;
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xF029:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xF033:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xF065:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xE09E:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
            case 0xE0A1:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.programCounter = cpu.programCounter + 2;
                opfound = 1;
                break;
        }

        switch(cpu.opcode & 0x00FF)
        {
            case 0x00EE:
                printf("cpu opcode found: %x\n", cpu.opcode);
                cpu.stackPointer--;
                cpu.programCounter = cpu.stack[cpu.stackPointer];
                opfound = 1;
                break;
        }
        
        if(!opfound)
        {
            printf("opcode not found: %x\n", cpu.opcode);
            break;
        }

        SDL_UpdateWindowSurface(game.window);
    }

    SDL_DestroyWindow(game.window);
    SDL_Quit();

    return 0;
}
