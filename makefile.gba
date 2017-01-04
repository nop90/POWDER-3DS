#################################
# HAM Makefile
#################################

include $(HAMDIR)/system/master.mak

#
# Set the name of your desired GBA image name here
#
PROGNAME = powder

#
# Set a list of files you want to compile 
# Please keep these alphabetical.
# 
OFILES += action.o \
	  ai.o \
	  artifact.o \
	  assert.o \
	  buf.o \
	  build.o \
	  control.o \
	  creature.o \
	  credits.o \
	  dpdf_table.o \
	  encyclopedia.o \
	  encyc_support.o \
	  gfxengine.o \
	  glbdef.o \
	  grammar.o \
	  hiscore.o \
	  input.o \
	  intrinsic.o \
	  item.o \
	  license.o \
	  main.o \
	  map.o \
	  mobref.o \
	  msg.o \
	  name.o \
	  piety.o \
	  rand.o \
	  signpost.o \
	  smokestack.o \
	  speed.o \
	  sramstream.o \
	  stylus.o \
	  victory.o \
	  gfx/all_bitmaps.o \
	  rooms/allrooms.o

######################################
# Standard Makefile targets start here
######################################

all : premake $(PROGNAME).$(EXT) postmake

premake:
	${HOME}/bin/enummaker source.txt
	${HOME}/bin/txt2c LICENSE.TXT license.cpp
	${HOME}/bin/txt2c CREDITS.TXT credits.cpp
	${HOME}/bin/encyclopedia2c encyclopedia.txt

postmake: powder.gba
	${HOME}/bin/gbafix powder.gba -tPOWDER`cat VERSION.TXT`


#
# Most Makefile targets are predefined for you, suchas
# vba, clean ... in the following file
#
include $(HAMDIR)/system/standard-targets.mak

######################################
# custom  Makefile targets start here
######################################


gfx: 
	cd gfx
	gfx2gba -t8 -m -fsrc *.bmp
	
clean:
	rm -f *.o *.i *.ii *.s
	rm -f gfx/*.o
	rm -f rooms/*.o
	rm -f glbdef.cpp glbdef.h
	rm -f license.cpp
	rm -f credits.cpp
	rm -f encyclopedia.cpp
	rm -f encyclopedia.h
