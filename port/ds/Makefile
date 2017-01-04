#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

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
LIBS	:= -lfat -lnds9
 
 
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)
#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

INCLUDE	:=	-I$(LIBNDS)/include -I$(CURDIR)
 
LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
 
CFLAGS	:=	-g -Wall -O1\
 			-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
			-ffast-math -fno-short-enums \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9 -DUSING_DS
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -mno-fpu -Wl,-Map,$(notdir $*.map)

 
#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
#---------------------------------------------------------------------------------
 
OUTPUT	:=	$(CURDIR)/$(TARGET)
 
VPATH	:=	

DEPSDIR	:=	$(CURDIR)

CFILES		:=	
CPPFILES	:=	dsmain.cpp \
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
$(OUTPUT).ds.gba	: 	$(OUTPUT).nds
# $(OUTPUT).nds	: 	$(OUTPUT).arm9

$(OUTPUT).nds: $(OUTPUT).arm9
	ndstool -c powder.nds -7 $(DEVKITPRO)/libnds/basic.arm7 -9 powder.arm9 -b ../../gfx/icon_nds.bmp "POWDER `cat ../../VERSION.TXT`; By Jeff Lait; "

$(OUTPUT).arm9	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)
 
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)
 
 
-include $(DEPENDS)

#---------------------------------------------------------------------------------
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	rm -f $(OFILES) $(DEPENDS) $(TARGET).elf $(TARGET).nds $(TARGET).arm9 $(TARGET).ds.gba 
 
