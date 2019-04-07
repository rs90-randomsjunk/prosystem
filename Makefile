CHAINPREFIX=/opt/mipsel-linux-uclibc
CROSS_COMPILE=$(CHAINPREFIX)/usr/bin/mipsel-linux-

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc

SYSROOT     := $(CHAINPREFIX)/usr/mipsel-buildroot-linux-uclibc/sysroot
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

# define regarding OS, which compiler to use
TARGET     = prosystem-od

TOOLCHAIN = /opt/gcw0-toolchain/usr
EXESUFFIX = .dge

# change compilation / linking flag options
F_OPTS = -falign-functions -falign-loops -falign-labels -falign-jumps \
	-ffast-math -fsingle-precision-constant -funsafe-math-optimizations \
	-fomit-frame-pointer -fno-builtin -fno-common \
	-fstrict-aliasing  -fexpensive-optimizations \
	-finline -finline-functions -fpeel-loops
CC_OPTS	= -O2 -mips32 -mhard-float -G0 -D_OPENDINGUX_ -D_VIDOD32_ $(F_OPTS)
CFLAGS      = $(SDL_CFLAGS) -DOPENDINGUX $(CC_OPTS)
LDFLAGS     = $(SDL_CFLAGS) $(CC_OPTS) -lSDL 

# Files to be compiled
SRCDIR    = ./emu/zlib  ./emu ./opendingux
VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS     = $(OBJ_C)

# Rules to make executable
$(TARGET)$(EXESUFFIX): $(OBJS)  
	$(LD) $(LDFLAGS) -o $(TARGET)$(EXESUFFIX) $^

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)$(EXESUFFIX) *.o
