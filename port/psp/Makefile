TARGET = powder

PSPSDK = $(shell psp-config --pspsdk-path)
PSPBIN = $(shell psp-config --psp-prefix)/bin
SDL_CONFIG = $(PSPBIN)/sdl-config

DEFAULT_CFLAGS = $(shell $(SDL_CONFIG) --cflags) 
PSP_FW_VERSION=371
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Powder
PSP_EBOOT_ICON= icon0.png
PSP_EBOOT_PIC1 = pic1.png

OBJS = pspmain.o \
	 hamfake.o \
	 ../../action.o \
	 ../../assert.o \
	 ../../ai.o \
	 ../../artifact.o \
	 ../../bmp.o \
	 ../../build.o \
	 ../../buf.o \
	 ../../control.o \
	 ../../creature.o \
	 ../../dpdf_table.o \
	 ../../encyc_support.o \
	 ../../gfxengine.o \
	 ../../grammar.o \
	 ../../hiscore.o \
	 ../../input.o \
	 ../../intrinsic.o \
	 ../../item.o \
	 ../../map.o \
	 ../../mobref.o \
	 ../../msg.o \
	 ../../name.o \
	 ../../piety.o \
	 ../../rand.o \
	 ../../signpost.o \
	 ../../smokestack.o \
	 ../../speed.o \
	 ../../sramstream.o \
	 ../../stylus.o \
	 ../../victory.o \
	 ../../encyclopedia.o \
	 ../../glbdef.o \
	 ../../credits.o \
	 ../../license.o \
	 ../../gfx/all_bitmaps.o \
	 ../../rooms/allrooms.o

#include ../../make/makerules.LINUX

CFLAGS = -O3 -g -G0 $(GLOBAL_CFLAGS) -Wall -DSYS_PSP $(INCLUDES) -I/usr/local/pspdev/psp/include/freetype2 \
					-I/usr/local/pspdev/psp/sdk/include -I/usr/local/pspdev/psp/include/SDL \
					-DLINUX -I ../../port/sdl \
					-I .
         
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS +=	$(shell $(SDL_CONFIG) --libs) \
	-lpsppower -lpspaudio -lpspgu -lpsphprm -lpsprtc -lstdc++ -lz -lm
	
		
include $(PSPSDK)/lib/build.mak

all: EBOOT.PBP

pclean:
	cd ../.. ; rm -f *.o
	cd ../../gfx ; rm -f *.o
	rm -f *.o
	cd ../sdl ; rm -f *.o
	rm -f $(TARGET)

premake:
	cd ../../rooms ; ./buildrooms.bash
	cd ../../gfx ; ./rebuild.sh
	cd ../.. ; support/enummaker/enummaker source.txt
	cd ../.. ; support/txt2c/txt2c LICENSE.TXT license.cpp
	cd ../.. ; support/txt2c/txt2c CREDITS.TXT credits.cpp
	cd ../.. ; support/encyclopedia2c/encyclopedia2c encyclopedia.txt
