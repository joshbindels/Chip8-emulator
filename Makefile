all: emu

emu: main.cpp
	clang++ -std=c++17 -lncurses main.cpp -o emu
