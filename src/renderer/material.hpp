#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>

bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const char* _name);
bgfx::ProgramHandle loadProgram(bx::FileReader* fr, const char* _vsName, const char* _fsName);

struct Material
{
	bgfx::ProgramHandle program;
};

