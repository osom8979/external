include $(SRC_PATH)build/arch.mk
SHAREDLIBSUFFIX = so
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX).$(FULL_VERSION)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX).$(SHAREDLIB_MAJORVERSION)
SHLDFLAGS = -Wl,-soname,$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXMAJORVER)
CFLAGS += -fPIC
LDFLAGS += -lpthread
STATIC_LDFLAGS += -lpthread -lm
ifeq ($(ASM_ARCH), x86)
ifeq ($(ARCH), x86_64)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf
endif
endif

