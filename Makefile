CHAINPREFIX=/opt/mipsel-linux-uclibc
CROSS_COMPILE=$(CHAINPREFIX)/usr/bin/mipsel-linux-

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc

SYSROOT     := $(CHAINPREFIX)/usr/mipsel-buildroot-linux-uclibc/sysroot
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

TARGET     = prosystem-od/prosystem-od.dge

# change compilation / linking flag options
F_OPTS = -falign-functions -falign-loops -falign-labels -falign-jumps \
	-ffast-math -fsingle-precision-constant -funsafe-math-optimizations \
	-fomit-frame-pointer -fno-builtin -fno-common \
	-fstrict-aliasing  -fexpensive-optimizations \
	-finline -finline-functions -fpeel-loops
CC_OPTS	= -O2 -mips32 -mhard-float -G0 -D_OPENDINGUX_ $(F_OPTS)
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

ipk: $(TARGET)
	@rm -rf /tmp/.prosystem-od-ipk/ && mkdir -p /tmp/.prosystem-od-ipk/root/home/retrofw/emus/prosystem-od /tmp/.prosystem-od-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators /tmp/.prosystem-od-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	@cp -r prosystem-od/prosystem-od.dge prosystem-od/prosystem-od.png prosystem-od/7800.rom /tmp/.prosystem-od-ipk/root/home/retrofw/emus/prosystem-od
	@cp prosystem-od/prosystem-od.lnk /tmp/.prosystem-od-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators
	@cp prosystem-od/atari7800.prosystem-od.lnk /tmp/.prosystem-od-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	@sed "s/^Version:.*/Version: $$(date +%Y%m%d)/" prosystem-od/control > /tmp/.prosystem-od-ipk/control
	@cp prosystem-od/conffiles /tmp/.prosystem-od-ipk/
	@tar --owner=0 --group=0 -czvf /tmp/.prosystem-od-ipk/control.tar.gz -C /tmp/.prosystem-od-ipk/ control conffiles
	@tar --owner=0 --group=0 -czvf /tmp/.prosystem-od-ipk/data.tar.gz -C /tmp/.prosystem-od-ipk/root/ .
	@echo 2.0 > /tmp/.prosystem-od-ipk/debian-binary
	@ar r prosystem-od/prosystem-od.ipk /tmp/.prosystem-od-ipk/control.tar.gz /tmp/.prosystem-od-ipk/data.tar.gz /tmp/.prosystem-od-ipk/debian-binary

clean:
	rm -f $(TARGET) *.o
