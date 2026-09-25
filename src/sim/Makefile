# Desktop simulator for the typing tutor core (host compiler, needs zlib).
ROOT    := ../..
BUILD   := $(ROOT)/build/sim
GEN     := $(ROOT)/build/gen
CORE    := $(wildcard ../core/*.cpp)
SRCS    := $(CORE) sim.cpp $(GEN)/fontdata.cpp
CXXFLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-parameter -std=c++17 -fno-exceptions -fno-rtti
CXXFLAGS += -I../core

$(BUILD)/sim: $(SRCS) $(wildcard ../core/*.h)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) -lz

$(GEN)/fontdata.cpp: $(ROOT)/scripts/gen_fonts.py $(wildcard $(ROOT)/assets/fonts/*.ttf)
	python3 $(ROOT)/scripts/gen_fonts.py $@

.PHONY: clean
clean:
	rm -rf $(BUILD)
