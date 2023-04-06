THISDIR:=$(dir $(lastword $(MAKEFILE_LIST)))
ifeq ($(OS),Windows_NT)
SLN_CFG=Release
BGFX_TARGET=win64_vs2022
SHADERC=$(THISDIR)../deps/bgfx/.build/$(BGFX_TARGET)/bin/shaderc$(SLN_CFG).exe
else
SHADERC=SHADERC
endif

SHADERS=vs_cubes.sc fs_cubes.sc
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

all: $(windows_spirv_SHADERS)

clean:
	$(RM) $(windows_spirv_SHADERS) *.bin.d

.PHONY: clean