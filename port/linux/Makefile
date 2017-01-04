TARGET = powder

OFILES = linuxmain.o \
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

PREMAKE = ../../support/map2c/map2c \
	  ../../support/tile2c/tile2c \
	  ../../support/bmp2c/bmp2c \
	  ../../support/encyclopedia2c/encyclopedia2c \
	  ../../support/enummaker/enummaker \
	  ../../support/txt2c/txt2c

all: $(TARGET)

powder: $(OFILES)
	$(CXX) -o $(TARGET) $(OFILES) `sdl-config --libs` $(CXXFLAGS) $(LDFLAGS)

.cpp.o:
	$(CXX) -DLINUX -I ../../port/sdl `sdl-config --cflags` -c $< -o $@ $(CXXFLAGS)

powder-static: premake $(OFILES)
	# This extra step is from:
	# http://www.trilithium.com/johan/2005/06/static-libstdc/
	# and is to try and ensure we end up with a static link
	# of stdc++.
	$(RM) libstdc++.a
	ln -s `$(CXX) -print-file-name=libstdc++.a`
	$(CXX) -static-libgcc -L. -o $(TARGET) $(OFILES) `sdl-config --libs` $(CXXFLAGS) $(LDFLAGS)

clean:
	$(RM) $(OFILES)
	$(RM) $(TARGET)
	@for s in $$( echo "$(PREMAKE)" | tr -s '' '\n'); do \
		$(MAKE) clean -C `dirname $$s`; \
	done;

$(PREMAKE):
	$(MAKE) -C `dirname $@`

premake: $(PREMAKE)
	cd ../../rooms ; ./buildrooms.bash
	cd ../../gfx ; ./rebuild.sh
	cd ../.. ; support/enummaker/enummaker source.txt
	cd ../.. ; support/txt2c/txt2c LICENSE.TXT license.cpp
	cd ../.. ; support/txt2c/txt2c CREDITS.TXT credits.cpp
	cd ../.. ; support/encyclopedia2c/encyclopedia2c encyclopedia.txt

