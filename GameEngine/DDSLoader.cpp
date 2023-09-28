#include "DDSLoader.h"

bool DDSLoader::load(const std::filesystem::path& path, std::uint32_t& width, std::uint32_t& height, VkFormat& format, std::vector<std::uint8_t>& data) {
	std::ifstream ifs(path, std::ios::in | std::ios::binary);
	if (!ifs) {
		std::cerr << "[DDSLoader] (" << path << ") failed to open file" << std::endl;
		return false;
	}

	Header header{};
	ifs.read((char*)&header, sizeof(Header));

	if (!(header.magic == 0x20534444 && header.size == 124)) {
		std::cerr << "[DDSLoader] (" << path << ") input file is not DDS file" << std::endl;
		return false;
	}

	width = header.width;
	height = header.height;

	// format check
	switch ((FourCC)header.fourCC) {
	case FourCC::DXT1:
		format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		break;
	case FourCC::DXT2:
	case FourCC::DXT3:
		format = VK_FORMAT_BC2_SRGB_BLOCK;
		break;
	case FourCC::DXT4:
	case FourCC::DXT5:
		format = VK_FORMAT_BC3_SRGB_BLOCK;
		break;
	default:
		std::cerr << "[DDSLoader] input file has unknown format" << std::endl;
		return false;
	}

	std::size_t blockSize = (FourCC)header.fourCC == FourCC::DXT1 ? 8 : 16;
	std::size_t blockWidth = header.width;
	std::size_t blockHeight = header.height;

	std::size_t mainImageSize = std::max<std::size_t>(1, (blockWidth + 3) / 4) * std::max<std::size_t>(1, (blockHeight + 3) / 4) * blockSize;
	
	data.resize(mainImageSize);

	ifs.read((char*)data.data(), mainImageSize);
}