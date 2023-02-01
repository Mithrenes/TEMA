TITLE_ID = TEMA00001
TARGET = TEMA
PSVITAIP = 192.168.0.0
PROJECT_TITLE := TEMA
OBJS   = src/main.o src/font.o src/graphics.o src/init.o src/net.o \
	src/package_installer.o src/archive.o src/file.o \
	src/utils.o src/sha1.o minizip/unzip.o minizip/ioapi.o \
	src/sfo.o src/vita_sqlite.o sqlite-3.6.23.1/sqlite3.o src/appdb.o

LIBS = -lSceDisplay_stub -lSceSysmodule_stub -lSceNet_stub \
    -lvita2d \
    -lSceDisplay_stub \
    -lSceCommonDialog_stub \
	-lSceNetCtl_stub -lSceHttp_stub -lSceAppMgr_stub -lSceAppUtil_stub \
	-lScePower_stub -lScePromoterUtil_stub \
	-lSceLibKernel_stub \
    -lSceGxm_stub \
    -lSceCtrl_stub \
    -lScePgf_stub \
    -lScePvf_stub \
    -lfreetype \
    -lz \
    -lm

DEFINES = -DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0 -Isqlite-3.6.23.1/

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -Wl,-q -O3 -std=c99 $(DEFINES)
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: eboot.bin
	vita-mksfoex -d PARENTAL_LEVEL=1 -s APP_VER=01.40 -s TITLE_ID=$(TITLE_ID) "$(PROJECT_TITLE)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin -a res/icon0.png=sce_sys/icon0.png \
	-a res/template.xml=sce_sys/livearea/contents/template.xml \
	-a res/startup.png=sce_sys/livearea/contents/startup.png \
	-a res/bg0.png=sce_sys/livearea/contents/bg0.png $@
	

eboot.bin: $(TARGET).velf
	vita-make-fself $< eboot.bin

$(TARGET).velf: $(TARGET).elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS) param.sfo eboot.bin

vpksend: $(TARGET).vpk
	curl -T $(TARGET).vpk ftp://$(PSVITAIP):1337/ux0:/
	@echo "Sent."

send: eboot.bin
	curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/$(TITLE_ID)/
	@echo "Sent."
