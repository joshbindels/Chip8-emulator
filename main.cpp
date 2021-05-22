#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <ncurses.h>
#include <stack>
#include <chrono>

#define FONT_MEM_START 0

uint8_t Memory[4000] = {0};     // RAM
volatile bool Display[64*32] = {0};   // Graphics
uint16_t PC = 0;                 // Program counter
uint16_t I = 0;                 // Index register
std::stack<uint16_t> Stack;
uint8_t DelayTimer = 0;
uint8_t SoundTimer = 0;
uint8_t V[16] = {0};            // Registers V0-V15


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
	wmove(win, 1, 1); 		// reset cursor
	for(int i = 0; i<(64*32); i++)
	{
		waddch(win, Display[i] ? ACS_BLOCK : ' ');
	}
}

int main(int argc, char** argv)
{
	initscr();
	nodelay(stdscr, TRUE); 		// getch non-blocking
	curs_set(0); 				// hide cursor
	WINDOW* win = newwin(34, 66, 5, 5);
	// Load font into RAM from address 0x00 - 0x50
	LoadFont(Memory);

	while(1)
	{
		if(getch() == 113) { break; }
		box(win, 0, 0);
		wrefresh(win);
		Draw(win);
	}

	delwin(win);
	endwin();


    return EXIT_SUCCESS;
}

