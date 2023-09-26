#include "utf16to8.h"

std::vector<uint32_t> utf16to32(const std::vector<std::uint8_t>& utf16bytes) {
	constexpr uint16_t high_begin = 0xd800;
	constexpr uint16_t high_end = 0xd8ff;
	constexpr uint16_t low_begin = 0xdc00;
	constexpr uint16_t low_end = 0xdfff;
	constexpr int shift = 10;
	constexpr uint32_t base = 0x0010000;

	std::vector<uint32_t> utf32bytes{};
	utf32bytes.reserve(utf16bytes.size());

	for (auto iter = utf16bytes.begin(); iter != utf16bytes.end();) {
		std::uint16_t c1 = *iter++ | (*iter++ << 8);

		if (iter != utf16bytes.end()) {
			if (high_begin <= c1 && c1 <= high_end) {
				std::uint16_t c2 = *iter++ | (*iter++ << 8);
				if (low_begin <= c2 && c2 <= low_end) {
					utf32bytes.push_back(static_cast<std::uint32_t>(((c1 - high_begin) << shift) + (c2 - low_begin) + base));
				}
				else {
					//utf32bytes.push_back(static_cast<std::uint32_t>(0xfffd));
					std::cerr << "failed to convert from UTF16 to UTF32" << std::endl;
					std::exit(EXIT_FAILURE);
				}
			}
			else if (low_begin <= c1 && c1 <= low_end) {
				//utf32bytes.push_back(static_cast<std::uint32_t>(0xfffd));
				std::cerr << "failed to convert from UTF16 to UTF32" << std::endl;
				std::exit(EXIT_FAILURE);
			}
			else {
				utf32bytes.push_back(static_cast<std::uint32_t>(c1));
			}
		}
		else if (low_begin <= c1 && c1 <= low_end) {
			//utf32bytes.push_back(static_cast<std::uint32_t>(0xfffd));
			std::cerr << "failed to convert from UTF16 to UTF32" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		else {
			utf32bytes.push_back(static_cast<std::uint32_t>(c1));
		}
	}


	return utf32bytes;
}

std::vector<std::uint8_t> utf32to8(std::vector<std::uint32_t>&& utf32bytes) {
	constexpr std::array<std::uint8_t, 7> first_byte_mark = {
		0x00,
		0x00,
		0xc0,
		0xe0,
		0xf0,
		0xf8,
		0xfc,
	};

	constexpr std::uint32_t utf32_max = 0x0010ffff;
	constexpr std::uint8_t byte_mark = 0x80;
	constexpr std::uint8_t byte_mask = 0xbf;

	std::vector<std::uint8_t> utf8bytes_rev{};
	utf8bytes_rev.reserve(utf32bytes.size() * 3);

	for (auto iter = utf32bytes.rbegin(); iter != utf32bytes.rend(); iter++) {
		std::uint32_t c = *iter;
		std::uint8_t num_bytes{};
		if (c < 0x80) num_bytes = 1;
		else if (c < 0x800) num_bytes = 2;
		else if (c < 0x10000) num_bytes = 3;
		else if (c < utf32_max) num_bytes = 4;
		else {
			num_bytes = 3;
			//c = 0xfffd;
			std::cerr << "failed to convert from UTF32 to UTF8" << std::endl;
			std::exit(EXIT_FAILURE);
		}

		switch (num_bytes) {
		case 4:
			utf8bytes_rev.push_back(static_cast<std::uint8_t>((c | byte_mark) & byte_mask));
			c >>= 6;
			[[fallthrough]];
		case 3:
			utf8bytes_rev.push_back(static_cast<std::uint8_t>((c | byte_mark) & byte_mask));
			c >>= 6;
			[[fallthrough]];
		case 2:
			utf8bytes_rev.push_back(static_cast<std::uint8_t>((c | byte_mark) & byte_mask));
			c >>= 6;
			[[fallthrough]];
		case 1:
			utf8bytes_rev.push_back(static_cast<std::uint8_t>(c | first_byte_mark[num_bytes]));
		}
	}

	return std::move(std::vector<std::uint8_t>{utf8bytes_rev.rbegin(), utf8bytes_rev.rend()});
}

std::vector<uint8_t> utf16to8(const std::vector<uint8_t>& utf16bytes) {
	auto v = utf32to8(utf16to32(utf16bytes));
	v.push_back('\0');
	return v;
}