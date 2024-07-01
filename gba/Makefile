#
# Template tonc makefile
#
# Yoinked mostly from DKP's template
#

# === SETUP ===========================================================

# --- No implicit rules ---
.SUFFIXES:

# --- Paths ---
export TONCLIB := ${DEVKITPRO}/libtonc
export LIBGBA  := ${DEVKITPRO}/libgba

# === TONC RULES ======================================================
#
# Yes, this is almost, but not quite, completely like to
# DKP's base_rules and gba_rules
#

export PATH	:=	$(DEVKITARM)/bin:$(PATH)


# --- Executable names ---

PREFIX		?=	arm-none-eabi-

export CC	:=	$(PREFIX)gcc
export CXX	:=	$(PREFIX)g++
export AS	:=	$(PREFIX)as
export AR	:=	$(PREFIX)ar
export NM	:=	$(PREFIX)nm
export OBJCOPY	:=	$(PREFIX)objcopy

# LD defined in Makefile


# === LINK / TRANSLATE ================================================

%.gba : %.elf
	@$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)
	@gbafix $@ -t$(GAMETITLE) -c$(GAMECODE) -mGAMEMAKER

#----------------------------------------------------------------------

%.mb.elf :
	@echo Linking multiboot
	$(LD) -specs=gba_mb.specs $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	$(NM) -Sn $@ > $(basename $(notdir $@)).map

#----------------------------------------------------------------------

%.elf :
	@echo Linking cartridge
	$(LD) -specs=gba.specs $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	$(NM) -Sn $@ > $(basename $(notdir $@)).map

#----------------------------------------------------------------------

%.a :
	@echo $(notdir $@)
	@rm -f $@
	$(AR) -crs $@ $^


# === OBJECTIFY =======================================================

%.iwram.o : %.iwram.cpp
	@echo $(notdir $<)
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(IARCH) -c $< -o $@

#----------------------------------------------------------------------
%.iwram.o : %.iwram.c
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(IARCH) -c $< -o $@

#----------------------------------------------------------------------

%.o : %.cpp
	@echo $(notdir $<)
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(RARCH) -c $< -o $@

#----------------------------------------------------------------------

%.o : %.c
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(RARCH) -c $< -o $@

#----------------------------------------------------------------------

%.o : %.s
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

#----------------------------------------------------------------------

%.o : %.S
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@


#----------------------------------------------------------------------
# canned command sequence for binary data
#----------------------------------------------------------------------

define bin2o
	bin2s $< | $(AS) -o $(@)
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(<F) | tr . _)`.h
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(<F) | tr . _)`.h
	echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(<F) | tr . _)`.h
endef
# =====================================================================

# --- Main path ---

export PATH	:=	$(DEVKITARM)/bin:$(PATH)


# === PROJECT DETAILS =================================================
# PROJ		: Base project name
# TITLE		: Title for ROM header (12 characters)
# LIBS		: Libraries to use, formatted as list for linker flags
# BUILD		: Directory for build process temporaries. Should NOT be empty!
# SRCDIRS	: List of source file directories
# DATADIRS	: List of data file directories
# INCDIRS	: List of header file directories
# LIBDIRS	: List of library directories
# General note: use `.' for the current dir, don't leave the lists empty.

export PROJ	?= $(notdir $(CURDIR))
TITLE		:= $(PROJ)

LIBS		:= -ltonc -lgba

BUILD		:= build
SRCDIRS		:= src \
							src/gsmplayer \
							src/gsmplayer/core \
							src/data/_compiled_files
DATADIRS	:= data
INCDIRS		:= src
LIBDIRS		:= $(TONCLIB) $(LIBGBA)

# --- switches ---

bMB		:= 1	# Multiboot build
bTEMPS	:= 0	# Save gcc temporaries (.i and .s files)
bDEBUG2	:= 0	# Generate debug info (bDEBUG2? Not a full DEBUG flag. Yet)


# === BUILD FLAGS =====================================================
# This is probably where you can stop editing
# NOTE: I've noticed that -fgcse and -ftree-loop-optimize sometimes muck
#	up things (gcse seems fond of building masks inside a loop instead of
#	outside them for example). Removing them sometimes helps

# --- Architecture ---

ARCH    := -mthumb-interwork -mthumb
RARCH   := -mthumb-interwork -mthumb
IARCH   := -mthumb-interwork -marm -mlong-calls

# --- Main flags ---

CFLAGS		:= -mcpu=arm7tdmi -mtune=arm7tdmi -Ofast
CFLAGS		+= -Wall
CFLAGS		+= $(INCLUDE)
CFLAGS		+= -ffast-math -fno-strict-aliasing

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS		:= $(ARCH) $(INCLUDE)
LDFLAGS 	:= $(ARCH) -Wl,-Map,$(PROJ).map

# --- switched additions ----------------------------------------------

# --- Multiboot ? ---
ifeq ($(strip $(bMB)), 1)
	TARGET	:= $(PROJ).mb
else
	TARGET	:= $(PROJ)
endif

# --- Save temporary files ? ---
ifeq ($(strip $(bTEMPS)), 1)
	CFLAGS		+= -save-temps
	CXXFLAGS	+= -save-temps
endif

# --- Debug info ? ---

ifeq ($(strip $(bDEBUG)), 1)
	CFLAGS		+= -DDEBUG -g
	CXXFLAGS	+= -DDEBUG -g
	ASFLAGS		+= -DDEBUG -g
	LDFLAGS		+= -g
else
	CFLAGS		+= -DNDEBUG
	CXXFLAGS	+= -DNDEBUG
	ASFLAGS		+= -DNDEBUG
endif

# --- Custom vars ? ---

GAMETITLE=gba-remote-play
GAMEMAKER=AGB
GAMECODE=AZCE # Megaman Zero (SRAM - 64kb)

# CXXFLAGS += -DCUSTOM_VAR_DEFINE

# === BUILD PROC ======================================================

ifneq ($(BUILD),$(notdir $(CURDIR)))

# Still in main dir:
# * Define/export some extra variables
# * Invoke this file again from the build dir
# PONDER: what happens if BUILD == "" ?

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=									\
	$(foreach dir, $(SRCDIRS) , $(CURDIR)/$(dir))	\
	$(foreach dir, $(DATADIRS), $(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

# --- List source and data files ---

CFILES		:=	$(foreach dir, $(SRCDIRS) , $(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir, $(SRCDIRS) , $(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir, $(SRCDIRS) , $(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir, $(DATADIRS), $(notdir $(wildcard $(dir)/*.*)))

# --- Set linker depending on C++ file existence ---
ifeq ($(strip $(CPPFILES)),)
	export LD	:= $(CC)
else
	export LD	:= $(CXX)
endif

# --- Define object file list ---
export OFILES	:=	$(addsuffix .o, $(BINFILES))					\
					$(CFILES:.c=.o) $(CPPFILES:.cpp=.o)				\
					$(SFILES:.s=.o)

# --- Create include and library search paths ---
export INCLUDE	:=	$(foreach dir,$(INCDIRS),-I$(CURDIR)/$(dir))	\
					$(foreach dir,$(LIBDIRS),-I$(dir)/include)		\
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	-L$(CURDIR) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# --- Create BUILD if necessary, and run this makefile from there ---

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	arm-none-eabi-nm -Sn $(OUTPUT).elf > $(BUILD)/$(TARGET).map

all	: $(BUILD)

clean:
	@echo clean ...
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).gba $(TARGET).sav


else		# If we're here, we should be in the BUILD dir

DEPENDS	:=	$(OFILES:.o=.d)

# --- Main targets ----

$(OUTPUT).gba	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)

-include $(DEPENDS)


endif		# End BUILD switch

# --- More targets ----------------------------------------------------

.PHONY: check-env clean rebuild start

check-env:
ifndef GBARP_OUT_DIR
	$(warning Missing environment variable: GBARP_OUT_DIR. See README.md)
	$(error "Aborting")
endif

rebuild: check-env clean $(BUILD)

start: check-env $(BUILD)
	start "$(TARGET).gba"

restart: check-env rebuild
	cp "$(TARGET).gba" "$(GBARP_OUT_DIR)"

# EOF
