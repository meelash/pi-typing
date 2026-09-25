# Desktop simulator for the typing tutor core (host compiler, needs zlib).
ROOT    := ../..
BUILD   := $(ROOT)/build/sim
GEN     := $(ROOT)/build/gen
CORE    := $(wildcard ../core/*.cpp)
SRCS    := $(CORE) sim.cpp $(GEN)/fontdata.cpp
HARFBUZZ := $(ROOT)/external/harfbuzz/src
CXXFLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-parameter -std=c++17 -fno-exceptions -fno-rtti
CXXFLAGS += -I../core -I$(HARFBUZZ) -I$(ROOT)/external/stb
# Same HarfBuzz configuration as the Pi kernel (src/pi/Makefile).
HB_DEFINES := -DHB_LEAN -DHB_MINI -DHB_NO_MT -DHB_NO_SVG -DHB_NO_UCD_UNASSIGNED -DNDEBUG

$(BUILD)/sim: $(SRCS) $(wildcard ../core/*.h) $(BUILD)/harfbuzz.o
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(BUILD)/harfbuzz.o -lz

# Built once: it takes a while and never changes.
$(BUILD)/harfbuzz.o: $(HARFBUZZ)/harfbuzz.cc
	@mkdir -p $(BUILD)
	$(CXX) -O2 -std=c++17 -fno-exceptions -fno-rtti $(HB_DEFINES) -c -o $@ $<

$(GEN)/fontdata.cpp: $(ROOT)/scripts/gen_fonts.py $(wildcard $(ROOT)/assets/fonts/*.ttf)
	python3 $(ROOT)/scripts/gen_fonts.py $@

.PHONY: clean
clean:
	rm -rf $(BUILD)
