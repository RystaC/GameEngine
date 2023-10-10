#include "GLSLCompiler.h"

bool GLSLCompiler::compile(const std::filesystem::path& path) {
	// require .glsl extension
	if (path.extension() != ".glsl") {
		std::cerr << "[GLSLCompiler] (" << path << ") input file is not a GLSL file (require .glsl extension)" << std::endl;
		return false;
	}

	// extract shader kind (assume file name is [name].[type].glsl)
	auto shaderKindName = path.stem().extension();
	shaderc_shader_kind shaderKind{};

	if (shaderKindName == ".vert") shaderKind = shaderc_shader_kind::shaderc_vertex_shader;
	else if (shaderKindName == ".tesc") shaderKind = shaderc_shader_kind::shaderc_tess_control_shader;
	else if (shaderKindName == ".tese") shaderKind = shaderc_shader_kind::shaderc_tess_evaluation_shader;
	else if (shaderKindName == ".geom") shaderKind = shaderc_shader_kind::shaderc_geometry_shader;
	else if (shaderKindName == ".frag") shaderKind = shaderc_shader_kind::shaderc_fragment_shader;
	else if (shaderKindName == ".comp") shaderKind = shaderc_shader_kind::shaderc_compute_shader;
	else if (shaderKindName == ".mesh") shaderKind = shaderc_shader_kind::shaderc_mesh_shader;
	else if (shaderKindName == ".task") shaderKind = shaderc_shader_kind::shaderc_task_shader;
	else if (shaderKindName == ".rgen") shaderKind = shaderc_shader_kind::shaderc_raygen_shader;
	else if (shaderKindName == ".rint") shaderKind = shaderc_shader_kind::shaderc_intersection_shader;
	else if (shaderKindName == ".rahit") shaderKind = shaderc_shader_kind::shaderc_anyhit_shader;
	else if (shaderKindName == ".rchit") shaderKind = shaderc_shader_kind::shaderc_closesthit_shader;
	else if (shaderKindName == ".rmiss") shaderKind = shaderc_shader_kind::shaderc_miss_shader;
	else if (shaderKindName == ".rcall") shaderKind = shaderc_shader_kind::shaderc_callable_shader;
	else {
		std::cerr << "[GLSLCompiler] (" << path <<") input file is unknown shader kind (require shader kind before extension)" << std::endl;
		return false;
	}

	// check time stamp
	{
		std::filesystem::file_time_type lastCompiledTime{};

		auto timeStampPath = path.stem();
		timeStampPath.concat(".stamp");

		std::ifstream timeStampFile(timeStampPath);
		if (timeStampFile) {
			timeStampFile.read((char*)&lastCompiledTime, sizeof(std::filesystem::file_time_type));

			// compare: last compile time > last update time?
			// -> no need to compile
			if (lastCompiledTime > std::filesystem::last_write_time(path)) {
				std::cerr << "[GLSLCompiler] (" << path << ") file has no changes from last compilation -> compilation skipped" << std::endl;
				return true;
			}
		}
	}

	// shader compile
	{
		std::cerr << "[GLSLCompiler] (" << path << ") compiling GLSL file" << std::endl;

		shaderc::Compiler compiler{};
		shaderc::CompileOptions options{};

		std::ifstream shaderFile(path);
		if (shaderFile.fail()) {
			std::cerr << "[GLSLCompiler] (" << path << ") failed to open file" << std::endl;
			return false;
		}

		std::string shaderSource{ std::istreambuf_iterator<char>(shaderFile), std::istreambuf_iterator<char>() };

		auto compiled = compiler.CompileGlslToSpv(shaderSource, shaderKind, path.generic_string().c_str());
		if (compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::cerr << "[GLSLCompiler] GLSL compilation failed:" << std::endl << compiled.GetErrorMessage() << std::endl;
			return false;
		}

		auto shaderSPIRVPath = path.stem();
		shaderSPIRVPath.concat(".spv");

		std::ofstream shaderSPIRV(shaderSPIRVPath, std::ios::out | std::ios::binary);
		for (const auto value : compiled) shaderSPIRV.write(reinterpret_cast<const char*>(&value), sizeof(std::uint32_t));
	}

	// save time stamp
	{
		auto timeStampPath = path.stem();
		timeStampPath.concat(".stamp");

		std::ofstream timeStamp(timeStampPath, std::ios::out | std::ios::binary);
		auto compiledTime = std::filesystem::file_time_type::clock::now();
		timeStamp.write((const char*)&compiledTime, sizeof(std::filesystem::file_time_type));
	}

	return true;
}