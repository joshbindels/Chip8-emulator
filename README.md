# CHIP-8 Emulator

 A CHIP-8 Emulator written in C++ with ncurses graphics.

![<Gif demo>](imgs/invaders.gif)
## Compilation

The emulator requires only C++17 (with boost).

The program can be compiled using the Makefile or from the command-line with the command:
```bash
clang++ -std=c++17 -lncurses main.cpp -o emu
```

## Running the Emulator
To run the emulator, execute the emu binary with the Ch8 file as argument
```bash
>>./emu <ch8 file>
```


## License
[MIT](https://choosealicense.com/licenses/mit/)
