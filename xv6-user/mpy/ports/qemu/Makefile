################################################################################
# Initial setup of Makefile environment

BOARD ?= MPS2_AN385

# Make the build directory reflect the board.
BUILD ?= build-$(BOARD)

include ../../py/mkenv.mk
-include mpconfigport.mk

# Include board specific .mk file.
include boards/$(BOARD).mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

ifeq ($(QEMU_ARCH),arm)
FROZEN_MANIFEST ?= "freeze('test-frzmpy')"
endif
ifeq ($(QEMU_ARCH),riscv32)
FROZEN_MANIFEST ?= "freeze('test-frzmpy', ('frozen_const.py', 'frozen_viper.py', 'native_frozen_align.py'))"
endif

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

################################################################################
# ARM specific settings

ifeq ($(QEMU_ARCH),arm)

CROSS_COMPILE ?= arm-none-eabi-

LDFLAGS += -nostdlib
LIBS = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

SRC_C += \
	mcu/arm/startup.c \
	shared/runtime/semihosting_arm.c \

endif

################################################################################
# RISC-V 64-bit specific settings

ifeq ($(QEMU_ARCH),riscv32)

CROSS_COMPILE ?= /opt/riscv_custom/bin/riscv64-unknown-elf-

GCC_VERSION = $(word 1, $(subst ., , $(shell $(CC) -dumpversion)))

RV64_ABI = lp64

QEMU_ARGS += -bios none

RV64_ARCH ?= rv64i_zicsr_zifencei

AFLAGS += -mabi=$(RV64_ABI) -march=$(RV64_ARCH)
CFLAGS += $(AFLAGS)
LDFLAGS += -mabi=$(RV64_ABI) -march=$(RV64_ARCH) -Wl,-EL


endif


################################################################################
# Project specific settings and compiler/linker flags

QEMU_SYSTEM = qemu-system-$(QEMU_ARCH)
QEMU_ARGS += -machine $(QEMU_MACHINE) -nographic -monitor null -semihosting
QEMU_ARGS += $(QEMU_EXTRA)

# Specifying QEMU_DEBUG=1 will block qemu until a debugger is connected.
ifeq ($(QEMU_DEBUG),1)
QEMU_DEBUG_ARGS ?= -s
QEMU_ARGS += -S $(QEMU_DEBUG_ARGS) $(QEMU_DEBUG_EXTRA)
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

CFLAGS += -DMICROPY_HW_BOARD_NAME='"$(QEMU_MACHINE)"'
CFLAGS += $(INC) -Wall -Wpointer-arith -Wdouble-promotion -Wfloat-conversion -Werror -std=gnu99 $(COPT) \
	 -ffunction-sections -fdata-sections
CFLAGS += $(CFLAGS_EXTRA)  -Wpointer-to-int-cast

LDFLAGS += -T $(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(@:.elf=.map)

# Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -g
COPT = -O0
else
COPT += -Os -DNDEBUG
endif

# If Picolibc is available then select it explicitly.  Ubuntu 22.04 ships its
# bare metal RISC-V toolchain with Picolibc rather than Newlib, and the default
# is "nosys" so a value must be provided.  To avoid having per-distro
# workarounds, always select Picolibc if available.
PICOLIBC_SPECS = $(shell $(CC) --print-file-name=picolibc.specs)
ifeq ($(PICOLIBC_SPECS),picolibc.specs)
# Picolibc was not found.
else
$(info picolibc used $(PICOLIBC_SPECS))
SPECS_FRAGMENT = --specs=$(PICOLIBC_SPECS)
CFLAGS += $(SPECS_FRAGMENT)
LDFLAGS += $(SPECS_FRAGMENT)
endif

################################################################################
# Source files and libraries

SRC_C += \
	main.c \
	uart.c \
	mphalport.c \
	shared/libc/string0.c \
	shared/readline/readline.c \
	shared/runtime/interrupt_char.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/sys_stdio_mphal.c \

LIB_SRC_C += $(SRC_LIB_LIBM_C)
LIB_SRC_C += $(SRC_LIB_LIBM_SQRT_SW_C)

OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)

################################################################################
# Main targets

all: $(BUILD)/firmware.elf

.PHONY: repl
repl: $(BUILD)/firmware.elf
	$(ECHO) "Use machine.reset() to exit"
	$(QEMU_SYSTEM) $(QEMU_ARGS) -serial mon:stdio -kernel $<

.PHONY: run
run: $(BUILD)/firmware.elf
	$(QEMU_SYSTEM) $(QEMU_ARGS) -serial pty -kernel $<

.PHONY: test
test: $(BUILD)/firmware.elf
	$(eval DIRNAME=ports/$(notdir $(CURDIR)))
	cd $(TOP)/tests && ./run-tests.py --target qemu --device execpty:"$(QEMU_SYSTEM) $(QEMU_ARGS) -serial pty -kernel ../$(DIRNAME)/$<" $(RUN_TESTS_ARGS) $(RUN_TESTS_EXTRA)

$(BUILD)/firmware.elf: $(LDSCRIPT) $(OBJ)
	$(Q)$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) $@

################################################################################
# Remaining make rules

include $(TOP)/py/mkrules.mk
