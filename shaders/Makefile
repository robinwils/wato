THISDIR:=$(dir $(lastword $(MAKEFILE_LIST)))
WATO_ROOT:=$(THISDIR)/..

ifeq ($(DEBUG),1)
	SHADERCFLAGS=--disasm --debug
	BUILD_TYPE=Debug
else
	SHADERCFLAGS=--disasm
	BUILD_TYPE=Release
endif

ifeq ($(OS),Windows_NT)
	BGFX_TARGET=x64-windows-static-md
	EXT=".exe"
else
	UNAME_S=$(shell uname -s)
	UNAME_M=$(shell uname -m)
	ifeq ($(UNAME_M),x86_64)
		ARCH=x64
	else ifeq ($(findstring armv8,$(UNAME_M)),armv8)
		ARCH=arm64
	else ifeq ($(findstring aarch64,$(UNAME_M)),aarch64)
		ARCH=arm64
	else
		ARCH=unknown
	endif

	ifeq ($(UNAME_S),Linux)
		BGFX_TARGET=$(ARCH)-linux
	else ifeq ($(UNAME_S),Darwin)
		BGFX_TARGET=$(ARCH)-osx
	endif
endif

BGFX_PACKAGE=$(WATO_ROOT)/vcpkg/packages/bgfx_$(BGFX_TARGET)
SHADERC=$(BGFX_PACKAGE)/tools/bgfx/shaderc$(EXT)

SHADERS=$(wildcard vs_*.sc) $(wildcard fs_*.sc) $(wildcard cs_*.sc)

define SHADER_TARGET =
$(1)_$(2)_SHADERS=$$(patsubst %.sc,%_$(1)_$(2).bin,$$(SHADERS))

vs_%_$(1)_$(2).bin: vs_%.sc
	@echo [$$(<)]
	$$(SHADERC) -i $$(BGFX_PACKAGE)/include/bgfx --platform $(1) -p $(2) --type vertex --depends -o $$(@) -f $$(<) $$(SHADERCFLAGS)

fs_%_$(1)_$(2).bin: fs_%.sc
	@echo [$$(<)]
	$$(SHADERC) -i $$(BGFX_PACKAGE)/include/bgfx --platform $(1) -p $(2) --type fragment --depends -o $$(@) -f $$(<) $$(SHADERCFLAGS)

cs_%_$(1)_$(2).bin: cs_%.sc
	@echo [$$(<)]
	$$(SHADERC) -i $$(BGFX_PACKAGE)/include/bgfx --platform $(1) -p $(2) --type compute --depends -o $$(@) -f $$(<) $$(SHADERCFLAGS)

-include $$(patsubst %.sc,%_$(1)_$(2).bin.d,$$(SHADERS))
endef

$(eval $(call SHADER_TARGET,windows,spirv))
$(eval $(call SHADER_TARGET,linux,spirv))
$(eval $(call SHADER_TARGET,linux,440))
$(eval $(call SHADER_TARGET,osx,metal))

linux_glsl_SHADERS=$(patsubst %.sc,%_linux_glsl.bin,$(SHADERS))
%_glsl.bin: %_440.bin
	mv $< $@

windows-shaders: $(windows_spirv_SHADERS)
linux-shaders: $(linux_spirv_SHADERS) $(linux_glsl_SHADERS)
osx-shaders: $(osx_metal_SHADERS)
all: windows-shaders linux-shaders osx-shaders

clean:
	$(RM) $(windows_spirv_SHADERS) $(linux_spirv_SHADERS) $(linux_glsl_SHADERS) $(linux_440_SHADERS) $(osx_metal_SHADERS) *.bin.d

.PHONY: clean
