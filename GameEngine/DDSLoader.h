#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#define MAKE_FOURCC(x, y, z, w) (((w) << 24) | ((z) << 16) | ((y) << 8) | (x))

class DDSLoader {
	// type aliases for win32
	using Dword = std::uint32_t;

	// header
	struct Header {
		Dword magic;
		Dword size;
		Dword flags;
		Dword height;
		Dword width;
		Dword pitchOrLinearSize;

		Dword depth;
		Dword mipmapCount;
		Dword reserved[11];
		Dword pfSize;
		Dword pfFlags;
		Dword fourCC;

		Dword rgbBitCount;
		Dword rBitMask, gBitMask, bBitMask, aBitMask;
		Dword caps, caps2;
		Dword reservedCaps[2];
		Dword reserved2;
	};

	enum HeaderFlagBit {
		H_CAPS   = 0x00000001,
		H_HEIGHT = 0x00000002,
		H_WIDTH  = 0x00000004,
		H_PITCH  = 0x00000008,
		H_FORMAT = 0x00001000,
		H_MIPMAP = 0x00020000,
		H_LINEAR = 0x00080000,
		H_DEPTH  = 0x00800000,
	};

	enum PixelFormatFlagBit {
		PF_ALPHA     = 0x00000001,
		PF_ALPHAONLY = 0x00000002,
		PF_FOURCC    = 0x00000004,
		PF_INDEX4    = 0x00000008,
		PF_INDEX8    = 0x00000020,
		PF_RGB       = 0x00000040,
		PF_LUMINANCE = 0x00020000,
		PF_BUMP      = 0x00080000,
	};

	enum class FourCC {
		DX10 = MAKE_FOURCC('D', 'X', '1', '0'),
		DXT1 = MAKE_FOURCC('D', 'X', 'T', '1'),
		DXT2 = MAKE_FOURCC('D', 'X', 'T', '2'),
		DXT3 = MAKE_FOURCC('D', 'X', 'T', '3'),
		DXT4 = MAKE_FOURCC('D', 'X', 'T', '4'),
		DXT5 = MAKE_FOURCC('D', 'X', 'T', '5'),
		BC4U = MAKE_FOURCC('B', 'C', '4', 'U'),
		BC4S = MAKE_FOURCC('B', 'C', '4', 'S'),
		BC5U = MAKE_FOURCC('B', 'C', '5', 'U'),
		BC5S = MAKE_FOURCC('B', 'C', '5', 'S'),

	};

	enum CapsFlagBit {
		ALPHA   = 0x00000002,
		COMPLEX = 0x00000008,
		TEXTURE = 0x00001000,
		MIPMAP  = 0x00400000,
	};

	enum Caps2FlagBit {
		CUBE       = 0x00000200,
		CUBE_POS_X = 0x00000400,
		CUBE_NEG_X = 0x00000800,
		CUBE_POS_Y = 0x00001000,
		CUBE_NEG_Y = 0x00002000,
		CUBE_POS_Z = 0x00004000,
		CUBE_NEG_Z = 0x00008000,
		VOLUME     = 0x00400000,
	};

	struct ExtendHeader {
		Dword format;
		Dword dimension;
		Dword miscFlag;
		Dword arraySize;
		Dword miscFlag2;
	};

	enum class DDSDimension {
		DIM_1D = 2,
		DIM_2D = 3,
		DIM_3D = 4,
	};

public:
	bool load(const std::filesystem::path& path, std::uint32_t& width, std::uint32_t& height, VkFormat& format, std::vector<std::uint8_t>& data);
};