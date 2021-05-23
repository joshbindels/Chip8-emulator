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
        //waddch(win, (Display[i] == 1) ? ACS_CKBOARD : ' ');
        waddch(win, (Display[i] == 1) ? ACS_BLOCK : ' ');
    }
    wmove(win, 0, 0);       // reset cursor
}

int translateKey(int k)
{
	switch(k)
	{
        case 49:
            return 0x1;
        case 50:
            return 0x2;
        case 51:
            return 0x3;
        case 52:
            return 0xC;
        case 113:
            return 0x4;
        case 119:
            return 0x5;
        case 101:
            return 0x6;
        case 111:
            return 0xD;
        case 97:
            return 0x7;
        case 115:
            return 0x8;
        case 100:
            return 0x9;
        case 102:
            return 0xE;
        case 122:
            return 0xA;
        case 120:
            return 0x0;
        case 99:
            return 0xB;
        case 118:
            return 0xF;
		default:
			return 0x0;
	}
}

int main(int argc, char** argv)
{
    initscr();
    nodelay(stdscr, TRUE);      // getch non-blocking
    noecho();
    curs_set(0);                // hide cursor
    WINDOW* win = newwin(32, 64, 5, 5);
    WINDOW* logwin = newwin(35, 70, 5, 80);
    scrollok(logwin, true);
    // Load font into RAM from address 0x00 - 0x50
    LoadFont(Memory);
    LoadCh8Program(argv[1]);
    auto next_frame = std::chrono::steady_clock::now();
    bool drawFlag = false;
    bool keyFlag = false;
    int keyIndex = 0;
    int keyCurrent = -1;
    char* logstr = (char*)malloc(sizeof("123\n"));;

    bool run = true;
    while(run)
    {
        next_frame += std::chrono::milliseconds(1000/75);

        int ch;
        switch(ch = getch())
        {
            case -1:
                break;
            case 27: // escape
                run = false;
                continue;
            default:
                keyCurrent = ch;
                sprintf(logstr, " %d\n", ch);
                waddstr(logwin, logstr);
                break;

        }
        //box(win, 0, 0);
        box(logwin, 0, 0);
        wrefresh(win);
        wrefresh(logwin);

        if(drawFlag) {
            Draw(win);
            drawFlag = false;
        }

        if(keyFlag)
        {
            if(keyCurrent != -1)
            {
                V[keyIndex] = translateKey(keyCurrent);
                keyFlag = false;
                keyIndex = 0;
                keyCurrent = -1;
            }
        }
        else {
        uint16_t opcode = Memory[PC] << 8 | Memory[PC+1];
        //sprintf(logstr, "0x%x\n", opcode);
        //waddstr(logwin, logstr);
        PC += 2;
        switch((opcode & 0xF000) >> 12)
        {
            case 0x0:
                switch(opcode & 0xFF) {
                    case 0xE0:
                        // clear screen (0x00E0)
                        for(int i=0; i<(64*32); i++) {
                            Display[i] = 0;
                        }
                        drawFlag = true;
                        break;
                    case 0xEE:
                        // return (0x00EE)
                        PC = Stack.top();
                        Stack.pop();
                        break;
                    default:
                        // call (0x0NNN)
                        break;
                }
                break;
            case 0x1:
                // goto (0x1NNN)
                PC = opcode & 0xFFF;
                break;
            case 0x2:
                // call (0x2NNN)
                Stack.push(PC);
                PC = opcode & 0xFFF;
                break;
            case 0x3:
                // cond eq (0x3XNN)
                if(V[(opcode & 0xF00) >> 8] == (opcode & 0xFF))
                {
                    PC += 2;
                }
                break;
            case 0x4:
                //cond neq (0x4XNN)
                if(V[(opcode & 0xF00) >> 8] != (opcode & 0xFF))
                {
                    PC += 2;
                }
                break;
            case 0x5:
                // cond eq register values (0x5XY0)
                if(V[(opcode & 0xF00) >> 8] == V[(opcode & 0xF0) >> 4])
                {
                    PC += 2;
                }
                break;
            case 0x6:
                // set register (0x6XNN)
                V[(opcode & 0xF00) >> 8] = opcode & 0x0FF;
                break;
            case 0x7:
                // add to register (0x7XNN)
                V[(opcode & 0xF00) >> 8] += opcode & 0x0FF;
                break;
            case 0x8:
                switch(opcode & 0xF) {
                    case 0x0:
                        // Vx = Vy (0x8XY0)
                        V[(opcode & 0xF00) >> 8] = V[(opcode & 0xF0) >> 4];
                        break;
                    case 0x1:
                        // Vx = Vx | Vy (0x8XY1)
                        V[(opcode & 0xF00) >> 8] |= V[(opcode & 0xF0) >> 4];
                        break;
                    case 0x2:
                        // Vx = Vx & Vy (0x8XY2)
                        V[(opcode & 0xF00) >> 8] &= V[(opcode & 0xF0) >> 4];
                        break;
                    case 0x3:
                        // Vx = Vx ^ Vy (0x8XY3)
                        V[(opcode & 0xF00) >> 8] ^= V[(opcode & 0xF0) >> 4];
                        break;
                    case 0x4:
                        // Vx += Vy (0x8XY4)
                        if((V[(opcode & 0xF00) >> 8] + V[(opcode & 0xF0) >> 4]) > 255)
                        {
                            // set overflow bit
                            V[0xF] = 1;
                        }
                        else
                        {
                            V[0xF] = 0;
                        }
                        V[(opcode & 0xF00) >> 8] += V[(opcode & 0xF0) >> 4];
                        break;
                    case 0x5:
                        //  Vx -= Vy (0x8XY5)
                        if((V[(opcode & 0xF0) >> 4] > V[(opcode & 0xF00) >> 8]))
                        {
                            // set underflow bit
                            V[0xF] = 0;
                        }
                        else
                        {
                            V[0xF] = 1;
                        }
                        V[(opcode & 0xF00) >> 8] -= V[(opcode & 0xF0) >> 4];
                        break;
                    case 0x6:
                        // Vx >>= 1 (0x8XY6)
                        V[0xF] = V[(opcode & 0xF00) >> 8] & 0x1;
                        V[(opcode & 0xF00) >> 8] >>= 1;
                        break;
                    case 0x7:
                        //  Vx = Vy - Vx (0x8XY7)
                        if((V[(opcode & 0xF00) >> 8] > V[(opcode & 0xF0) >> 4]))
                        {
                            // set underflow bit
                            V[0xF] = 0;
                        }
                        else
                        {
                            V[0xF] = 1;
                        }
                        V[(opcode & 0xF00) >> 8] = V[(opcode & 0xF0) >> 4] - V[(opcode & 0xF00) >> 8];
                        break;
                    case 0xE:
                        // Vx <<= 1 (0x8XYE)
                        V[0xF] = (V[(opcode & 0xF00) >> 8] & 0x80) >> 7;
                        V[(opcode & 0xF00) >> 8] <<= 1;
                        break;
                }
                break;
            case 0x9:
                // Vx != Vy (0x9XY0)
                if(V[(opcode & 0xF00) >> 8] != V[(opcode & 0xF0) >> 4])
                {
                    PC += 2;
                }
                break;
            case 0xA:
                // I = NNN (0xANNN)
                I = opcode & 0xFFF;
                break;
            case 0xB:
                // PC = V0 + NNN (0xBNNN)
                PC = V[0] + (opcode & 0xFFF);
                break;
            case 0xC:
                // Vx = rand() & NN (0xCXNN)
                V[(opcode & 0xF00) >> 8] = (rand() % 256) & (opcode & 0xFF);
                break;
            case 0xD:
                { // DXYN
                    //  draw (0xDXYN)
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
            case 0xE:
                switch(opcode & 0xFF)
                {
                    case 0x9E:
                        // key() == Vx (0xEX9E)
                        if(translateKey(keyCurrent) == V[(opcode & 0xF00) >> 8])
                        {
                            PC += 2;
                        }
                        break;
                    case 0xA1:
                        // key() != Vx (0xEXA1)
                        if(translateKey(keyCurrent) != V[(opcode & 0xF00) >> 8])
                        {
                            PC += 2;
                        }
                        break;
                }
                break;
            case 0xF:
                switch(opcode & 0xFF)
                {
                    case 0x07:
                        // Vx = DelayTimer (0xFX07)
                        V[(opcode & 0xF00) >> 8] = DelayTimer;
                        break;
                    case 0x0A:
                        // Vx = get_key() blocking (0xFX0A)
                        // TODO: this is very ugly, clean it up
                        keyFlag = true;
                        keyIndex = (opcode & 0xF00) >> 8;
                        break;
                    case 0x15:
                        // DelayTimer = Vx (0xFX15)
                        DelayTimer = V[(opcode & 0xF00) >> 8];
                        break;
                    case 0x18:
                        // Soundtimer = Vx (0xFX18)
                        SoundTimer = V[(opcode & 0xF00) >> 8];
                        break;
                    case 0x1E:
                        // I += Vx (0xFX1E)
                        I += V[(opcode & 0xF00) >> 8];
                        break;
                    case 0x29:
                        // Set I to Font for Vx(0xFX29)
                        //TODO
                        I = (V[(opcode & 0xF00) >> 8] * 5) + FONT_MEM_START;

                        break;
                    case 0x33:
                        {
                            // set BCD (0xFX33)
                            uint8_t val = V[(opcode & 0xF00) >> 8];
                            uint8_t one = val % 10;
                            uint8_t ten = (val % 100)/10;
                            uint8_t hun = (val % 1000)/100;
                            Memory[I+0] = hun;
                            Memory[I+1] = ten;
                            Memory[I+2] = one;
                            break;
                        }
                    case 0x55:
                        {
                            // reg_dump(Vx, &I) (0xFX55)
                            for(int i=0; i<=((opcode & 0xF00) >> 8); i++)
                            {
                                Memory[I+i] = V[i];
                            }
                            break;
                        }
                    case 0x65:
                        {
                            // reg_load(Vx, &I) (0xFX65)
                            for(int i=0; i<=((opcode & 0xF00) >> 8); i++)
                            {
                                V[i] = Memory[I+i];
                            }
                            break;
                        }
                }
                break;
            default:
                break;
        }
        }

        std::this_thread::sleep_until(next_frame);

    }

    delwin(win);
    delwin(logwin);
    endwin();


    return EXIT_SUCCESS;
}

