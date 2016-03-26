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

CRUNCHMAKE = crunchMake $(shell pkg-config --cflags --libs crunch++)
ifeq ($(BUILD_VERBOSE), 0)
	CRUNCHMAKE += -q
endif
CRUNCH = crunch++

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib

O = crc32.o stream.o conversions.o reader.o
SO = libAPNG.so
TESTS = testAPNG.so

DEPS = .dep

quiet_cmd_crunchMake = -n
cmd_crunchMake = $(CRUNCHMAKE) $(2)
quiet_cmd_crunch = -n
cmd_crunch = $(CRUNCH) $(2)

default: all

all: $(DEPS) $(SO)

.dep:
	$(call run-cmd,install_dir,.dep)

$(SO): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call debug-strip,$(SO))

%.o: %.cpp $(DEPS)
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

%.o: %.cc $(DEPS)
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

%.o: %.cxx $(DEPS)
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

#install:
#	$(cal run-cmd,install_bin,$(EXE),$(PATH))

test: $(O) $(TESTS)

testAPNG.so: CRUNCHMAKE += $(addprefix -o,$(O))
$(TESTS): $(subst .so,.cxx,$@)
	$(call run-cmd,crunchMake,$(subst .so,.cxx,$@))

check: test
	$(call run-cmd,crunch,$(subst .so,,$(TESTS)))

clean: $(DEPS)
	$(call run-cmd,rm,APNG,$(O) $(SO))
	$(call run-cmd,rm,tests,$(TESTS))
	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all clean test check install
.SUFFIXES: .cxx .so .o

-include .dep/*.d
