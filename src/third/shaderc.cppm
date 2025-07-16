module;

#include <shaderc/shaderc.hpp>

export module shaderc;

export namespace shaderc {
    using target_env = ::shaderc_target_env;
    using env_version = ::shaderc_env_version;
    using spirv_version = ::shaderc_spirv_version;
    using compilation_status = ::shaderc_compilation_status;
    using source_language = ::shaderc_source_language;
    using shader_kind = ::shaderc_shader_kind;
    using profile = ::shaderc_profile;
    using optimization_level = ::shaderc_optimization_level;
    using limit = ::shaderc_limit;
    using uniform_kind = ::shaderc_uniform_kind;
    using include_result = ::shaderc_include_result;
    using include_type = ::shaderc_include_type;
    using shaderc::CompilationResult;
    using shaderc::Compiler;
    using shaderc::CompileOptions;
    using shaderc::SpvCompilationResult;
    using shaderc::AssemblyCompilationResult;
    using shaderc::PreprocessedSourceCompilationResult;
}


