TARGET = powder


OFILES = macmain.o \
	 SDLMain.o \
	 ../sdl/hamfake.o \
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

include ../../make/makerules.OSX

clean:
	rm -f $(OFILES)
	rm -f $(TARGET)

premake:
	cd ../../rooms ; ./buildrooms.bash
	cd ../../gfx ; ./rebuild.sh
	cd ../.. ; support/enummaker/enummaker source.txt
	cd ../.. ; support/txt2c/txt2c LICENSE.TXT license.cpp
	cd ../.. ; support/txt2c/txt2c CREDITS.TXT credits.cpp
	cd ../.. ; support/encyclopedia2c/encyclopedia2c encyclopedia.txt
