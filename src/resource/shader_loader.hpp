#include <bx/file.h>

#include <memory>
#include <unordered_map>

#include "renderer/shader.hpp"

struct ShaderLoader final {
    using uniform_desc_map = std::unordered_map<std::string, UniformDesc>;
    using result_type      = std::shared_ptr<Shader>;

    result_type
    operator()(const char* aVsName, const char* aFsName, const uniform_desc_map& aUniforms);

   private:
    bx::FileReader mfr;
};
