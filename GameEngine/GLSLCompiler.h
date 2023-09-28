#pragma once

#include <shaderc/shaderc.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

class GLSLCompiler {

public:
	bool compile(const std::filesystem::path&);
};
