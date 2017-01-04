#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/gba_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	powder

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lgba
 
 
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBGBA)
#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

INCLUDE	:=	-I$(LIBGBA)/include -I$(CURDIR)
 
LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
 
CFLAGS	:=	-g -Wall -O1\
 			-mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer\
			-ffast-math -fno-short-enums \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DNO_HAM
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-g $(ARCH) -Wl,-Map,$(notdir $*.map)

 
#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
#---------------------------------------------------------------------------------
 
OUTPUT	:=	$(CURDIR)/$(TARGET)
 
VPATH	:=	

DEPSDIR	:=	$(CURDIR)

CFILES		:=	
CPPFILES	:=	gbamain.cpp \
			hamfake.cpp \
			../../action.cpp \
			../../assert.cpp \
			../../ai.cpp \
			../../artifact.cpp \
			../../bmp.cpp \
			../../build.cpp \
			../../buf.cpp \
			../../control.cpp \
			../../creature.cpp \
			../../dpdf_table.cpp \
			../../encyc_support.cpp \
			../../gfxengine.cpp \
			../../grammar.cpp \
			../../hiscore.cpp \
			../../input.cpp \
			../../intrinsic.cpp \
			../../item.cpp \
			../../map.cpp \
			../../mobref.cpp \
			../../msg.cpp \
			../../name.cpp \
			../../piety.cpp \
			../../rand.cpp \
			../../signpost.cpp \
			../../smokestack.cpp \
			../../speed.cpp \
			../../sramstream.cpp \
			../../stylus.cpp \
			../../victory.cpp \
			../../encyclopedia.cpp \
			../../glbdef.cpp \
			../../credits.cpp \
			../../license.cpp \
			../../gfx/all_bitmaps.cpp \
			../../rooms/allrooms.cpp

SFILES		:=	
BINFILES	:=	
 
#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
LD	:=	$(CXX)

OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
 
DEPENDS	:=	$(OFILES:.o=.d)
 
#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).gba	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)
 
#---------------------------------------------------------------------------------
%.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)
 
 
# We include this postfix rule as we want to include the version
# number in the ROM tag, not just the title.  And we are particular
# about our caps.
postmake: 
	gbafix powder.gba -tPOWDER`cat ../../VERSION.TXT`

-include $(DEPENDS)

#---------------------------------------------------------------------------------
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	rm -f $(OFILES) $(DEPENDS) $(TARGET).elf $(TARGET).gba 
 
