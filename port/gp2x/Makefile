TARGET = powder

GP2XSDK=/usr/local/gp2xdev
GP2XBIN=$(GP2XSDK)/bin
SDL_CONFIG = $(GP2XBIN)/sdl-config

DEFAULT_CFLAGS = $(shell $(SDL_CONFIG) --cflags)
CC=$(GP2XBIN)/gp2x-g++

OFILES = ../linux/linuxmain.o \
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

include ../../make/makerules.GP2X

CFLAGS = -O3 -g -I$(GP2XSDK)/include $(shell $(SDL_CONFIG) --cflags)

CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

LIBS += $(shell $(SDL_CONFIG) --static-libs) -static

clean:
	cd ../.. ; rm -f *.o
	cd ../../gfx ; rm -f *.o
	rm -f *.o
	cd ../sdl ; rm -f *.o
	cd ../linux ; rm -f *.o
	rm -f $(TARGET)

postmake:
	cp ../../CREDITS.TXT Powder2X/CREDITS.TXT
	cp ../../README.TXT Powder2X/README.TXT
	cp ../../LICENSE.TXT Powder2X/LICENSE.TXT
	cp powder Powder2X/pdata
