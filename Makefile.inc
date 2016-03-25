# Makefile include for APNG. Set V to something other than 0 for more detailed build output.

ifeq ($(MAKELEVEL), 0)

# Silence the tools
ifeq ("$(origin V)", "command line")
	BUILD_VERBOSE = $(V)
endif
ifndef BUILD_VERBOSE
	BUILD_VERBOSE = 0
endif

MAKEFLAGS += -w

ifeq ($(BUILD_VERBOSE), 0)
	quiet=quiet_
	Q=@
else
	quiet=
	Q=
endif
ifneq ($(findstring s,$(MAKEFLAGS)),)
	quiet=silent_
endif

export quiet Q BUILD_VERBOSE

endif

quiet_cmd_cxx = " CXX   $@"
cmd_cxx = $(CXX) $(2)
quiet_cmd_ccld = " CCLD  $@"
cmd_ccld = $(CXX) $(2)
cmd_makedep = $(1) $(2)
quiet_cmd_strip = " STRIP $@"
cmd_strip = $(STRIP) $(1)
quiet_cmd_rm = " CLEAN $(2)"
cmd_rm = $(RM) $(3)
quiet_cmd_ar = " AR    $(2)"
cmd_ar = $(AR) $(2) $(3)
quiet_cmd_ranlib = " RANLIB $(2)"
cmd_ranlib = $(RANLIB) $(2)
quiet_cmd_ln = " LN    $(3) => $(2)"
cmd_ln = $(LN) $(2) $(3)
quiet_cmd_chmod = " CHMOD $(2)"
cmd_chmod = $(CHMOD) $(2)
quiet_cmd_install = " INSTALL $(2)"
cmd_install = $(INSTALL) $(2)
quiet_cmd_install_dir = " INSTALL $(2)"
cmd_install_dir = $(INSTALL_DIR) $(2)
quiet_cmd_install_file = " INSTALL $(2) => $(3)"
cmd_install_file = $(INSTALL_FILE) $(2) $(3)
quiet_cmd_install_bin = " INSTALL $(2) => $(3)"
cmd_install_bin = $(INSTALL_BIN) $(2) $(3)
quiet_cmd_ldconfig = " LDCONFIG"
cmd_ldconfig = ldconfig

define run-cmd
	@echo $($(quiet)cmd_$(1))
	@$(cmd_$(1))
endef

define makedep
	@if [ -z "$(quiet)" ]; then \
		echo $(cmd_makedep); \
	fi
	@$(cmd_makedep)
	@if [ "$($dir $<)" != "./" ]; then \
		sed -i "s|^$(notdir $@):|$@:|" .dep/$*.d; \
	fi
endef

define debug-strip
	$(Q)if [ $(DEBUG) -eq 0 ]; then \
		echo $($(quiet)cmd_strip); \
		$(cmd_strip); \
	fi
endef

define ldconfig
	$(Q)if [ $(UID) -eq 0 ]; then \
		echo $($(quiet)cmd_ldconfig); \
		$(cmd_ldconfig); \
	fi
endef

# Set up build engine variables
GXX ?= g++
ifeq ($(strip $(DEBUG)), 1)
	OPTIM_FLAGS = -ggdb
	GCC_FLAGS =
else
	GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
	ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
		GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
	else
		GCC_FLAGS =
	endif
	OPTIM_FLAGS = -O2
	DEBUG = 0
endif
GCC_FLAGS += -fPIC -DPIC

ifeq ($(strip $(TEST)), 1)
	OPTIM_FLAGS += -fprofile-arcs -ftest-coverage
endif

CXX = $(GXX) $(GCC_FLAGS)
AR = ar cr
RANLIB = ranlib
LN = ln -sf
STRIP = strip -x
INSTALL = install
INSTALL_FILE = $(INSTALL) -m644
INSTALL_BIN = $(INSTALL) -m755
INSTALL_DIR = mkdir -p
UID = $(shell id -u)

export CC CXX DEBUG UID
