# SPDX-License-Identifier: GPL-2.0-only
# Based on external/gnu-efi/apps/Makefile

ARCH 		:= aarch64
CROSS_COMPILE 	:=

SRCDIR		:= $(CURDIR)
OUTDIR		:= $(CURDIR)/out

GNUEFI_DIR	:= $(SRCDIR)/external/gnu-efi
GNUEFI_OUT	:= $(GNUEFI_DIR)/$(ARCH)

LIBFDT_DIR	:= external/dtc/libfdt

export TOPDIR	?= $(GNUEFI_DIR)
include $(GNUEFI_DIR)/Make.defaults

CRTOBJS		= $(OBJDIR)/gnuefi/crt0-efi-$(ARCH).o
LDSCRIPT	= $(GNUEFI_DIR)/gnuefi/elf_$(ARCH)_efi.lds

EFI_LDFLAGS	+= -L$(GNUEFI_OUT)/lib -L$(GNUEFI_OUT)/gnuefi $(CRTOBJS)
EFI_LIBS	+= -T $(LDSCRIPT)

EFI_LDFLAGS	+= -pie -Bsymbolic -gc-sections

EFI_CFLAGS      += -fPIE -ffunction-sections -fdata-sections
EFI_LDFLAGS     += --no-dynamic-linker

EFI_LIBS	+= -lefi -lgnuefi

FORMAT		:= -O efi-bsdrv-$(ARCH)

QEBSPIL_OBJS := \
	src/event.o \
	src/fw.o \
	src/main.o \
	src/scm.o \
	src/external/cache.o \
	src/external/libc.o \

.PHONY: all
all: $(OUTDIR)/qebspilaa64.efi

include $(CURDIR)/$(LIBFDT_DIR)/Makefile.libfdt
QEBSPIL_OBJS += $(addprefix $(LIBFDT_DIR)/,$(LIBFDT_OBJS))
INCDIR += -I$(CURDIR)/$(LIBFDT_DIR)
EFI_CFLAGS += -D_POSIX_C_SOURCE=200809L
$(OUTDIR)/$(LIBFDT_DIR)/%.o: CFLAGS := $(CFLAGS) -Wno-unused-parameter

include $(GNUEFI_DIR)/Make.rules

$(CRTOBJS):
	@$(MAKE) -C$(GNUEFI_DIR) CROSS_COMPILE=$(CROSS_COMPILE) \
		CFLAGS="$(USER_CFLAGS) -ffunction-sections -fdata-sections" \
		lib gnuefi

$(OUTDIR)/%.o: %.c
	@$(ECHO) "  CC       $(subst $(OUTDIR)/,,$@)"
	@mkdir -p $(dir $@)
	$(HIDE)$(CC) $(INCDIR) $(CFLAGS) -c $< -o $@

$(OUTDIR)/qebspilaa64.so: $(addprefix $(OUTDIR)/,$(QEBSPIL_OBJS)) | $(CRTOBJS)
	@$(ECHO) "  LD       $(notdir $@)"
	$(HIDE)$(LD) $(LDFLAGS) $^ -o $@ $(EFI_LIBS)

.PHONY: clean
clean:
	rm -rf $(OUTDIR)
	$(MAKE) -C$(GNUEFI_DIR) ARCH=$(ARCH) clean
