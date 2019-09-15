#CC specifies which compiler we're using
CC = clang

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS =

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -Llib\x64

#CFLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
CFLAGS = -O3 -Wall -Wextra -Wno-unused-parameter

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lSDL2main -lSDL2 -lSDL2_ttf -Xlinker /subsystem:console -fprofile-instr-generate 

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = cnes.exe

cnes.exe : ic_6502.o ic_2c02.o main_sdl.o nes_header.o mappers.o apu.o debug.o
	clang $(LIBRARY_PATHS) $(LINKER_FLAGS) $^ -o $@



.PHONY : test clean

clean :
	del *.exe *.o
test : test.exe
	test.exe nestest.nes > mytest.log

main_test.o : ic_6502.o ic_2c02.o nes_header.o mappers.o apu.o

main_sdl.o : ic_6502.o ic_2c02.o nes_header.o mappers.o apu.o debug.o

ic_6502.o : CFLAGS +=-g -fprofile-instr-generate -fcoverage-mapping
ic_6502.o : mappers.o

ic_2c02.o : CFLAGS +=-g -fprofile-instr-generate -fcoverage-mapping
ic_2c02.o : mappers.o

test.exe : ic_6502.o ic_2c02.o main_test.o nes_header.o apu.o mappers.o
	clang $(LIBRARY_PATHS) $(LINKER_FLAGS) $^ -o $@

