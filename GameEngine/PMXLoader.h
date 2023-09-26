#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "utf16to8.h"

#define READ_FUNC(name, ...) void name(std::ifstream& ifs, __VA_ARGS__)

#define UTF8_TEXT(text_buf) (const char*)(text_buf).data()
#define UTF16_TEXT(text_buf) (const char*)utf16to8((text_buf)).data()

class PMXLoader {
	// text encode
	enum class Encode {
		UTF16LE,
		UTF8,
	};

	// type aliases
	using Byte = std::uint8_t;
	using sByte = std::int8_t;
	using uShort = std::uint16_t;
	using Short = std::int16_t;
	using uInt = std::uint32_t;
	using Int = std::int32_t;
	using Float = float;
	using Float2 = glm::vec2;
	using Float3 = glm::vec3;
	using Float4 = glm::vec4;
	using TextBuf = std::vector<Byte>;
	using BitFlag = Byte;

	// generalize index type
	using IndexType = Int;

	// index for size information
	enum class IndexSize {
		ENCODE = 0,
		ADDITIONAL_UV = 1,
		VERTEX = 2,
		TEXTURE = 3,
		MATERIAL = 4,
		BONE = 5,
		MORPH = 6,
		RIGID = 7,
	};

	// model information
	struct ModelInfo {
		TextBuf name, name_en;
		TextBuf comment, comment_en;
	};

	// vertex data
	struct Attribute {
		Float3 position;
		Float3 normal;
		Float2 uv;
	};

	enum class WeightType {
		BDEF1 = 0,
		BDEF2 = 1,
		BDEF4 = 2,
		SDEF = 3,
	};

	struct BDEF1 {
		IndexType bone1;
	};

	void readBDEF1(BDEF1& weight) {
		ifs.read((char*)&weight.bone1, getIndexSize(IndexSize::BONE));
	}

	struct BDEF2{
		IndexType bone1, bone2;
		Float weight1;
	};

	void readBDEF2(BDEF2& weight) {
		ifs.read((char*)&weight.bone1, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.bone2, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.weight1, sizeof(Float));
	}

	struct BDEF4{
		IndexType bone1, bone2, bone3, bone4;
		Float weight1, weight2, weight3, weight4;
	};

	void readBDEF4(BDEF4& weight) {
		ifs.read((char*)&weight.bone1, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.bone2, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.bone3, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.bone4, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.weight1, sizeof(Float));
		ifs.read((char*)&weight.weight2, sizeof(Float));
		ifs.read((char*)&weight.weight3, sizeof(Float));
		ifs.read((char*)&weight.weight4, sizeof(Float));
	}

	struct SDEF {
		IndexType bone1, bone2;
		Float weight1;
		Float3 sdef_c, sdef_r0, sdef_r1;
	};

	void readSDEF(SDEF& weight) {
		ifs.read((char*)&weight.bone1, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.bone2, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&weight.weight1, sizeof(Float));
		ifs.read((char*)&weight.sdef_c, sizeof(Float3));
		ifs.read((char*)&weight.sdef_r0, sizeof(Float3));
		ifs.read((char*)&weight.sdef_r1, sizeof(Float3));
	}

	struct BoneWeight {
		WeightType type;
		union {
			BDEF1 bdef1;
			BDEF2 bdef2;
			BDEF4 bdef4;
			SDEF sdef;
		} weight;
	};

	// materials
	struct MaterialColor {
		Float4 diffuse;
		Float3 specular;
		Float specCoef;
		Float3 ambient;
	};

	// morphs
	enum class MorphType {
		GROUP = 0,
		VERTEX = 1,
		BONE = 2,
		UV = 3,
		A_UV1 = 4,
		A_UV2 = 5,
		A_UV3 = 6,
		A_UV4 = 7,
		MATERIAL = 8,
		MATERIAL_MUL,
		MATERIAL_ADD,
	};

	struct GroupMorph {
		Float rate;
	};

	struct VertexMorph {
		Float3 offset;
	};

	struct UVMorph {
		Float4 offset;
	};

	struct BoneMorph {
		Float3 move;
		Float4 rotate_quat;
	};

	struct MaterialMorph {
		Float4 diffuse;
		Float3 specular;
		Float specCoef;
		Float3 ambient;
		Float4 edgeColor;
		Float edgeSize;
		Float4 textureCoef;
		Float4 sphereCoef;
		Float4 toonCoef;
	};

	struct Morph {
		MorphType type;
		IndexType index;
		union {
			GroupMorph group;
			VertexMorph vertex;
			UVMorph uv, a_uv1, a_uv2, a_uv3, a_uv4;
			BoneMorph bone;
			MaterialMorph material_mul, material_add;
		} data;
	};

	// frames
	enum class FrameTarget {
		BONE = 0,
		MORPH = 1,
	};

	struct FrameElement {
		FrameTarget target;
		union {
			IndexType bone, morph;
		} index;
	};
	// fields

	std::ifstream ifs;

	Byte info[8];

	// block read functions

	void readHeader();
	void readModelInfo();
	void readVertexData();
	void readFaceData();
	void readTextureData();
	void readMaterialData();
	void readBoneData();
	void readMorphData();
	void readFrameData();
	void readRigidData();
	void readJointData();

	// utility
	Byte getIndexSize(IndexSize target) {
		return info[static_cast<std::uint32_t>(target)];
	}

	void readTextBuf(TextBuf& textBuf) {
		Int textSize{};
		ifs.read((char*)&textSize, sizeof(Int));
		textBuf.resize(textSize);
		ifs.read((char*)textBuf.data(), sizeof(Byte) * textSize);
	}

public:
	void load(const std::filesystem::path&);
};