CHAINPREFIX=
CROSS_COMPILE=

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc

SYSROOT     := 
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

TARGET     = prosystem-od.dge

# change compilation / linking flag options
F_OPTS = 
CC_OPTS	= -O0 -g3 -D_OPENDINGUX_ $(F_OPTS)
CFLAGS      = $(SDL_CFLAGS) -DOPENDINGUX $(CC_OPTS)
LDFLAGS     = $(SDL_CFLAGS) $(CC_OPTS) -lSDL 

# Files to be compiled
SRCDIR  = ./emu/zlib ./emu ./opendingux
VPATH   = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS    = $(OBJ_C)

# Rules to make executable
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $(TARGET) $^

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o
