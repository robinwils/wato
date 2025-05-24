THISDIR:=$(dir $(lastword $(MAKEFILE_LIST)))
ifeq ($(OS),Windows_NT)
BUILD_TYPE=Release
BGFX_TARGET=win64_vs2022
EXT=".exe"
else
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
BUILD_TYPE=Debug
BGFX_TARGET=linux64_gcc
else ifeq ($(UNAME_S),Darwin)
BUILD_TYPE=Debug
BGFX_TARGET=osx64_gcc
endif
endif
SHADERC=$(THISDIR)../deps/bgfx/.build/$(BGFX_TARGET)/bin/shaderc$(BUILD_TYPE)$(EXT)

SHADERS=$(wildcard vs_*.sc) $(wildcard fs_*.sc) $(wildcard cs_*.sc)
SHADERC_INCLUDE=-i $(THISDIR)../deps/bgfx/src/ -i $(THISDIR)../deps/bgfx/examples/

define SHADER_TARGET =
$(1)_$(2)_SHADERS=$$(patsubst %.sc,%_$(1)_$(2).bin,$$(SHADERS))

vs_%_$(1)_$(2).bin: vs_%.sc
	@echo [$$(<)]
	$$(SHADERC) $$(SHADERC_INCLUDE) --platform $(1) -p $(2) --type vertex --depends -o $$(@) -f $$(<) --disasm

fs_%_$(1)_$(2).bin: fs_%.sc
	@echo [$$(<)]
	$$(SHADERC) $$(SHADERC_INCLUDE) --platform $(1) -p $(2) --type fragment --depends -o $$(@) -f $$(<) --disasm

cs_%_$(1)_$(2).bin: cs_%.sc
	@echo [$$(<)]
	$$(SHADERC) $$(SHADERC_INCLUDE) --platform $(1) -p $(2) --type compute --depends -o $$(@) -f $$(<) --disasm

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
