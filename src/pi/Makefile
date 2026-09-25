#
# Bare-metal kernel for the Raspberry Pi (built with Circle).
# Use the top-level Makefile; it prepares a patched copy of Circle and builds its libraries first.
#

CIRCLEHOME ?= ../../build/circle-pi
BUILD      ?= ../../build/pi
GEN        = ../../build/gen

CORE_SRCS  = $(wildcard ../core/*.cpp)
OBJS       = $(BUILD)/main.o $(BUILD)/kernel.o $(BUILD)/usbinput.o $(BUILD)/logbuffer.o $(BUILD)/libc.o \
	     $(patsubst ../core/%.cpp,$(BUILD)/core/%.o,$(CORE_SRCS)) \
	     $(BUILD)/core/fontdata.o $(BUILD)/harfbuzz.o

# HarfBuzz (Urdu Nastaliq shaping) and stb_truetype (glyph rasteriser), pinned submodules.
HARFBUZZ   = ../../external/harfbuzz/src
STB        = ../../external/stb
# OpenType shaping only: no threads, files, environment, AAT or variable fonts.
HB_DEFINES = -DHB_LEAN -DHB_MINI -DHB_NO_MT -DHB_NO_SVG -DHB_NO_UCD_UNASSIGNED -DNDEBUG

LIBS       = $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	     $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	     $(CIRCLEHOME)/lib/usb/libusb.a \
	     $(CIRCLEHOME)/lib/input/libinput.a \
	     $(CIRCLEHOME)/lib/fs/libfs.a \
	     $(CIRCLEHOME)/lib/sound/libsound.a \
	     $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

CPPFLAGS += $(EXTRA_DEFINES)
NASTALIQ_INCLUDE = -I$(HARFBUZZ) -I$(STB)

# Objects live in $(BUILD) so the Pi and QEMU variants never mix.
$(BUILD)/%.o: %.cpp $(wildcard *.h) $(wildcard ../core/*.h)
	@mkdir -p $(dir $@)
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -c -o $@ $<

# The core is shared with the desktop simulator.
$(BUILD)/core/%.o: ../core/%.cpp $(wildcard ../core/*.h)
	@mkdir -p $(dir $@)
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -c -o $@ $<

$(BUILD)/core/nastaliq.o: ../core/nastaliq.cpp $(wildcard ../core/*.h)
	@mkdir -p $(dir $@)
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) $(NASTALIQ_INCLUDE) -c -o $@ $<

# HarfBuzz needs the C++ standard headers (header-only use), which Circle's
# flags exclude, so it gets its own flags. Its C library needs are met by
# Circle and libc.cpp.
$(BUILD)/harfbuzz.o: $(HARFBUZZ)/harfbuzz.cc
	@mkdir -p $(dir $@)
	@echo "  CPP   $@"
	@$(CPP) $(ARCH) -O2 -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics -fsigned-char \
		$(HB_DEFINES) -c -o $@ $<

$(BUILD)/core/fontdata.o: $(GEN)/fontdata.cpp ../core/font.h
	@mkdir -p $(dir $@)
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -I../core -c -o $@ $<

$(GEN)/fontdata.cpp: ../../scripts/gen_fonts.py $(wildcard ../../assets/fonts/*.ttf)
	python3 ../../scripts/gen_fonts.py $@

-include $(DEPS)
