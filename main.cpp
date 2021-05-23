#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <ncurses.h>
#include <stack>
#include <chrono>
#include <thread>
#include <fstream>

#define FONT_MEM_START 0

uint8_t Memory[4000] = {0};     // RAM
uint8_t Display[64*32] = {0};   // Graphics
uint16_t PC = 0x200;                 // Program counter
uint16_t I = 0;                 // Index register
std::stack<uint16_t> Stack;
uint8_t DelayTimer = 0;
uint8_t SoundTimer = 0;
uint8_t V[16] = {0};            // Registers V0-V15


void LoadCh8Program(const std::string& fn)
{
    std::streampos size;
    char* memblock;
    std::ifstream file(fn, std::ios::in|std::ios::binary|std::ios::ate);
    if(file.is_open()) {
        size = file.tellg();
        memblock = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(memblock, size);
        file.close();
        for(int i=0; i<size; i++) {
            Memory[0x200+i] = (uint8_t)memblock[i];
        }

        delete[] memblock;
    }

}

void LoadFont(uint8_t* mem, int addr=0)
{
    for(int f: {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    })
    {
        mem[addr++] = f;
    }
}

void Draw(WINDOW* win)
{
    for(int i = 0; i<(64*32); i++)
    {
        waddch(win, (Display[i] == 1) ? ACS_CKBOARD : ' ');
    }
    wmove(win, 0, 0);       // reset cursor
}

int main(int argc, char** argv)
{
    initscr();
    nodelay(stdscr, TRUE);      // getch non-blocking
    curs_set(0);                // hide cursor
    WINDOW* win = newwin(32, 64, 5, 5);
    WINDOW* logwin = newwin(35, 70, 5, 80);
    // Load font into RAM from address 0x00 - 0x50
    LoadFont(Memory);
    LoadCh8Program(argv[1]);
    auto next_frame = std::chrono::steady_clock::now();
    bool drawFlag = false;

    while(1)
    {
        next_frame += std::chrono::milliseconds(1000/60);

        if(getch() == 113) { break; }
        box(win, 0, 0);
        box(logwin, 0, 0);
        wrefresh(win);
        wrefresh(logwin);
        if(drawFlag) {
            Draw(win);
            drawFlag = false;
        }


        uint16_t opcode = Memory[PC] << 8 | Memory[PC+1];
        PC += 2;
        switch(opcode & 0xF000)
        {
            case 0x000:
                break;
            case 0x1000: // 1NNN Jump
                PC = opcode & 0xFFF;
                break;
            case 0x6000: // 6XNN
                V[(opcode & 0xF00) >> 8] = opcode & 0x0FF;
                break;
            case 0x7000: // 7XNN
                V[(opcode & 0xF00) >> 8] += opcode & 0x0FF;
                break;
            case 0xA000: // ANNN
                I = opcode & 0xFFF;
                break;
            case 0xD000: // DXYN
                {
                uint8_t x = V[(opcode & 0xF00) >> 8] % 64;
                uint8_t y = V[(opcode & 0x0F0) >> 4] % 32;
                uint8_t height = opcode & 0x00F;
                uint8_t pixel;
                V[0xF] = 0;

                for(int yline = 0; yline < height; yline++) {
                    pixel = Memory[I + yline];
                    for(int xline = 0; xline < 8; xline++) {
                        // check byte 1 bit at a time
                        if((pixel & (0x80 >> xline)) != 0) {
                            // check bit for flag
                            if(Display[(x + xline + ((y + yline) * 64))] == 1) {
                                V[0xF] = 1;
                            }
                            // toggle bit
                            Display[x + xline + ((y + yline) * 64)] ^= 1;
                        }
                    }

                }
                drawFlag = true;
                break;
                }
            default:
                break;
        }

        std::this_thread::sleep_until(next_frame);

    }

    delwin(win);
    delwin(logwin);
    endwin();


    return EXIT_SUCCESS;
}

