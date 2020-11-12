TITLE_ID = SKGF0RM47
TARGET   = storageFormat
OBJS     = main.o graphics.o font.o

LIBS = -lSceLibKernel_stub -lSceVshBridge_stub -lSceDisplay_stub -lSceAppMgr_stub -ltaihen_stub -lScePower_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -Wall -O3
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin -a user.suprx=user.suprx -a kernel.skprx=kernel.skprx -a icon0.png=sce_sys/icon0.png $@

eboot.bin: $(TARGET).velf
	vita-make-fself -a 0x2800000000000001 $< $@

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).vpk $(TARGET).velf $(TARGET).elf $(OBJS) \
		eboot.bin param.sfo