#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

struct PMX_Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	// additional UV
	glm::vec4 additionalUV[4];
	// bones
	glm::ivec4 boneIndices;
	glm::vec4 boneWeights;
	// glm::vec3 sdef_c, sdef_r0, sdef_r1;
	glm::float32_t edgeMult;
};

using PMX_Indices = std::variant<std::vector<std::uint16_t>, std::vector<std::uint32_t>>;

using PMX_TexturePath = std::filesystem::path;

struct PMX_Material {
	// color
	glm::vec4 diffuse;
	glm::vec3 specular;
	glm::float32_t specCoef;
	glm::vec3 ambient;

	std::uint8_t flags;

	// edge
	glm::vec4 edgeColor;
	glm::float32_t edgeSize;

	// texture indices
	std::int32_t textureIndex;
	std::int32_t sphereIndex;
	std::uint8_t sphereMode;

	// toon texture
	std::uint8_t isSharedToon;
	std::int32_t toonIndex;

	// face
	std::uint32_t indexCount, indexOffset;
};

struct PMX_IK {
	std::int32_t index;
	std::uint8_t isLimited;
	glm::vec3 lowerBound_rad;
	glm::vec3 upperBound_rad;
};

struct PMX_Bone {
	glm::vec3 position;
	std::int32_t parentIndex;
	std::int32_t hierarchy;

	std::uint16_t flags;

	// IK
	struct {
		std::int32_t targetIndex;
		std::int32_t loopCount;
		glm::float32_t limit_rad;
		std::vector<PMX_IK> links;
	} ik;
};

struct PMX_Morph_Group {
	std::int32_t index;
	glm::float32_t rate;
};

struct PMX_Morph_Vertex {
	std::int32_t index;
	glm::vec3 offset;
};

struct PMX_Morph_Bone {
	std::int32_t index;
	glm::vec3 translate;
	glm::vec4 rotate_quat;
};

struct PMX_Morph_UV {
	std::int32_t index;
	glm::vec4 offset;
};

struct PMX_Morph_Material {
	std::int32_t index;
	glm::vec4 diffuse;
	glm::vec3 specular;
	glm::float32_t specCoef;
	glm::vec3 ambient;
	glm::vec4 edgeColor;
	glm::float32_t edgeSize;
	glm::vec4 textureCoef;
	glm::vec4 sphereCoef;
	glm::vec4 toonCoef;
};

struct PMX_Morphs {
	std::vector<PMX_Morph_Group> groupMorphs;
	std::vector<PMX_Morph_Vertex> vertexMorphs;
	std::vector<PMX_Morph_Bone> boneMorphs;
	std::vector<PMX_Morph_UV> uvMorphs;
	std::vector<PMX_Morph_UV> addUVMorphs1, addUVMorphs2, addUVMorphs3, addUVMorphs4;
	std::vector<PMX_Morph_Material> materialAddMorphs, materialMulMorphs;
};

struct PMX_Frame {
};

struct PMX_Rigid {
	std::int32_t index;
	std::uint8_t group;
	std::uint16_t groupFlag;
	std::uint8_t shape;
	glm::vec3 size;
	glm::vec3 position;
	glm::vec3 rotate_rad;

	glm::float32_t mass;
	glm::float32_t transAtte;
	glm::float32_t rotAtte;
	glm::float32_t reflection;
	glm::float32_t friction;
	std::uint8_t calcMode;
};

struct PMX_Joint {
	// 6-DOF (degree of freedom) only
	// std::uint8_t type
	std::int32_t indexA, indexB;

	glm::vec3 position;
	glm::vec3 rotate_rad;
	glm::vec3 transLower, transUpper;
	glm::vec3 rotLower_rad, rotUpper_rad;
	glm::vec3 springTrans;
	glm::vec3 springRot;
};

struct PMXData {
	std::vector<PMX_Vertex> vertices;
	PMX_Indices indices;
	std::vector<PMX_TexturePath> texturePaths;
	std::vector<PMX_Material> materials;
	std::vector<PMX_Bone> bones;
	PMX_Morphs morphs;
	// std::vector<PMX_Frame> frames;
	std::vector<PMX_Rigid> rigids;
	std::vector<PMX_Joint> joints;
};

class PMXLoader {
	// index for size information
	enum class Index {
		ENCODE = 0,
		ADDITIONAL_UV = 1,
		VERTEX = 2,
		TEXTURE = 3,
		MATERIAL = 4,
		BONE = 5,
		MORPH = 6,
		RIGID = 7,
	};

	// fields

	std::ifstream ifs_;
	std::filesystem::path parentPath_;

	std::uint8_t indexSize_[8];

	// block read functions

	void readHeader();
	void readModelInfo();
	void readVertexData(std::vector<PMX_Vertex>&);
	void readFaceData(PMX_Indices&);
	void readTextureData(std::vector<PMX_TexturePath>&);
	void readMaterialData(std::vector<PMX_Material>&);
	void readBoneData(std::vector<PMX_Bone>&);
	void readMorphData(PMX_Morphs&);
	void readFrameData(/*std::vector<PMX_Frame>&*/);
	void readRigidData(std::vector<PMX_Rigid>&);
	void readJointData(std::vector<PMX_Joint>&);

	// utility (inline)

	inline std::uint8_t getIndexSize(Index type) {
		return indexSize_[static_cast<std::size_t>(type)];
	}

#define READ_FUNC(name, type) inline void read_##name(type& val) { ifs_.read((char*)&val, sizeof(type)); } \
inline void skip_##name() { ifs_.seekg(sizeof(type), std::ios_base::cur); }

	READ_FUNC(Byte, std::uint8_t);
	READ_FUNC(sByte, std::int8_t);
	READ_FUNC(uShort, std::uint16_t);
	READ_FUNC(Short, std::int16_t);
	READ_FUNC(uInt, std::uint32_t);
	READ_FUNC(Int, std::int32_t);
	READ_FUNC(Float, glm::float32_t);
	READ_FUNC(Float2, glm::vec2);
	READ_FUNC(Float3, glm::vec3);
	READ_FUNC(Float4, glm::vec4);

#undef READ_FUNC

	inline void read_Index(std::uint8_t size, std::int32_t& index) {
		std::int32_t val{};
		ifs_.read((char*)&val, size);

		// if size is 4 byte -> directly use value
		if (size == 4) index = val;

		// check if index is -1
		// 1 byte -> 255, 2 byte -> 65535
		else if (val == ((1 << size * 8) - 1)) index = -1;
		else index = val;
	}

	inline void skip_Index(std::uint8_t size) {
		ifs_.seekg(size, std::ios_base::cur);
	}

	// maybe unused
	// uint8_t -> UTF8, uint16_t -> UTF16
	template<typename T, std::enable_if_t<std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::uint16_t>, std::nullptr_t> = nullptr>
	inline void read_TextBuf(std::vector<T>& textBuf) {
		std::int32_t length{};
		read_Int(length);
		textBuf.resize(length / sizeof(T));
		ifs_.read((char*)textBuf.data(), length);
	}

	inline void skip_TextBuf() {
		std::int32_t length{};
		read_Int(length);
		ifs_.seekg(length, std::ios_base::cur);
	}

public:
	void load(const std::filesystem::path& path, PMXData& data);
};
