#include "TexLoader.h"

bool TexLoader::load(const std::filesystem::path& path, VkExtent3D& size, VkFormat& format, std::vector<std::uint8_t>& data) {

	auto extension = path.extension();
	if (extension == ".jpg" || extension == ".png" || extension == ".bmp") {
		int x{}, y{}, c{};

		std::ifstream ifs(path, std::ios::in | std::ios::binary);
		if (!ifs) {
			std::cerr << "[TexLoader] (" << path << ") failed to open image" << std::endl;
			return false;
		}

		ifs.seekg(0, std::ios::end);
		auto fileSize = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		std::vector<uint8_t> dataBuffer(fileSize);
		ifs.read((char*)dataBuffer.data(), fileSize);
		auto imageData = stbi_load_from_memory(dataBuffer.data(), static_cast<int>(dataBuffer.size()), &x, &y, &c, STBI_rgb_alpha);
		if (!imageData) {
			std::cerr << "[TexLoader] failed to read image" << std::endl;
			return false;
		}
		size.width = static_cast<std::uint32_t>(x);
		size.height = static_cast<std::uint32_t>(y);
		size.depth = 1;

		format = VK_FORMAT_R8G8B8A8_SRGB;
		data = std::vector<std::uint8_t>(imageData, imageData + (x * y * STBI_rgb_alpha));
		stbi_image_free(imageData);
		return true;
	}
	else if (extension == ".dds") {
		DDSLoader loader{};
		std::uint32_t width{}, height{};
		size.depth = 1;
		return loader.load(path, size.width, size.height, format, data);
	}
	else {
		std::cerr << "[TexLoader] invalid format image" << std::endl;
		return false;
	}
}