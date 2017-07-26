include Makefile.inc

PKG_CONFIG_PKGS = zlib
CFLAGS_EXTRA = $(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
LIBS_EXTRA = $(shell pkg-config --libs $(PKG_CONFIG_PKGS))
DEFS = -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA)
CFLAGS = $(OPTIM_FLAGS) -c $(DEFS) -o $@ $<
DEPFLAGS = $(OPTIM_FLAGS) -E -MM $(DEFS) -o .dep/$*.d $<
LIBS = $(LIBS_EXTRA)
# -pthread
LFLAGS = $(OPTIM_FLAGS) -shared $(O) $(LIBS) -Wl,-soname,$@ -z defs -o $@

SED = sed -e 's:@LIBDIR@:$(LIBDIR):g' -e 's:@PREFIX@:$(PREFIX):g' -e 's:@VERSION@:$(VER):g'

CRUNCHMAKE = crunchMake $(shell pkg-config --cflags --libs crunch++ zlib)
ifeq ($(BUILD_VERBOSE), 0)
	CRUNCHMAKE += -q
endif
ifeq ($(strip $(COVERAGE)), 1)
	CRUNCHMAKE += -lgcov
endif
CRUNCH = crunch++

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib
PKGDIR = $(LIBDIR)/pkgconfig
INCDIR = $(PREFIX)/include/APNG

O = crc32.o stream.o conversions.o reader.o
H = apng.hxx stream.hxx
VERMAJ = .0
VERMIN = $(VERMAJ).0
VERREV = $(VERMIN).1
VER = $(VERREV)
SO = libAPNG.so
PC = libAPNG.pc
TESTS = testAPNG.so

DEPS = .dep

quiet_cmd_crunchMake = -n
cmd_crunchMake = $(CRUNCHMAKE) $(2)
quiet_cmd_crunch = -n
cmd_crunch = $(CRUNCH) $(2)

default: all

all: $(DEPS) $(SO)

.dep $(LIBDIR) $(PKGDIR) $(INCDIR):
	$(call run-cmd,install_dir,.dep)

install: all $(LIBDIR) $(PKGDIR) $(INCDIR) $(PC)
	$(call run-cmd,install_file,$(addsuffix $(VER),$(SO)),$(LIBDIR))
	$(call run-cmd,install_file,$(PC),$(PKGDIR))
	#$(call run-cmd,install_file,$(H),$(INCDIR))
	$(call run-cmd,ln,$(LIBDIR)/$(SO)$(VERREV),$(LIBDIR)/$(SO)$(VERMIN))
	$(call run-cmd,ln,$(LIBDIR)/$(SO)$(VERMIN),$(LIBDIR)/$(SO)$(VERMAJ))
	$(call run-cmd,ln,$(LIBDIR)/$(SO)$(VERMAJ),$(LIBDIR)/$(SO))
	$(call ldconfig)

$(SO): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call debug-strip,$(SO))
	$(call run-cmd,ln,$@,$@$(VER))

%.pc: %.pc.in
	$(call run-cmd,sed,$<,$@)

%.o: %.cxx | $(DEPS)
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

tests: $(O) $(TESTS)

testAPNG.so: CRUNCHMAKE += $(addprefix -o,$(O))
$(TESTS): $(subst .so,.cxx,$@)
	$(call run-cmd,crunchMake,$(subst .so,.cxx,$@))

check: tests
	$(call run-cmd,crunch,$(subst .so,,$(TESTS)))

clean: $(DEPS)
	$(call run-cmd,rm,APNG,$(O) $(SO))
	$(call run-cmd,rm,tests,$(TESTS))
	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all clean tests check install
.SUFFIXES: .cxx .so .o
-include .dep/*.d
