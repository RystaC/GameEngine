#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "stb_image.h"
#include "DDSLoader.h"

class TexLoader {

public:
	bool load(const std::filesystem::path&, VkExtent3D&, VkFormat&, std::vector<std::uint8_t>&);
};
