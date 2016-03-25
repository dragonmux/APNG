include Makefile.inc

PKG_CONFIG_PKGS =
CFLAGS_EXTRA = $(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
LIBS_EXTRA = $(shell pkg-config --libs $(PKG_CONFIG_PKGS))
DEFS = -Wall -Wextra -pedantic -std=c++11
CFLAGS = $(OPTIM_FLAGS) -c $(DEFS) -o $@ $<
# $(CFLAGS_EXTRA)
DEPFLAGS = $(OPTIM_FLAGS) -E -MM $(DEFS) -o .dep/$*.d $<
LIBS =
# $(LIBS_EXTRA) -pthread
LFLAGS = $(OPTIM_FLAGS) -shared $(O) $(LIBS) -Wl,-soname,$@ -z defs -o $@

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib

O = crc32.o stream.o conversions.o reader.o
SO = libAPNG.so

DEPS = .dep

default: all

all: $(DEPS) $(SO)

.dep:
	$(call run-cmd,install_dir,.dep)

$(O): $(DEPS)

$(SO): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call debug-strip,$(SO))

%.o: %.cpp
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

%.o: %.cc
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

%.o: %.cxx
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

#install:
#	$(cal run-cmd,install_bin,$(EXE),$(PATH))

clean: $(DEPS)
	$(call run-cmd,rm,APNG,$(O) $(SO))
	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all clean test check install

-include .dep/*.d