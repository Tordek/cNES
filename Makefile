#CC specifies which compiler we're using
CC = clang

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS =

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -Llib\x64

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
COMPILER_FLAGS = -Wall -Wextra

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lSDL2main -lSDL2 -Xlinker /subsystem:console

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = cnes.exe

cnes.exe : ic_6502.o ic_2c02.o main_sdl.o nes_header.o mappers.o apu.o
	clang $(LIBRARY_PATHS) $(LINKER_FLAGS) $^ -o $@

.PHONY : test
test : test.exe
	test.exe nestest.nes > mytest.log

main_test.o : ic_6502.o ic_2c02.o nes_header.o mappers.o apu.o

main_sdl.o : ic_6502.o ic_2c02.o nes_header.o mappers.o apu.o

ic_6502.o : bus.o

ic_2c02.o : bus.o

test.exe : ic_6502.o ic_2c02.o bus.o main_test.o nes_header.o ram.o
	clang $(LIBRARY_PATHS) $(LINKER_FLAGS) $^ -o $@

