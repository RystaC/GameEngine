#include "PMXLoader.h"

void PMXLoader::readHeader() {
	// magic number
	std::uint8_t magic[4]{};
	read_Byte(magic[0]);
	read_Byte(magic[1]);
	read_Byte(magic[2]);
	read_Byte(magic[3]);

	// must be "PMX "
	if (!(std::string_view((const char*)magic, 4) == "PMX ")) {
		std::cerr << "Input file is not a PMX file" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	glm::float32_t version{};
	read_Float(version);

	std::uint8_t dataSize{};
	read_Byte(dataSize);

	// must be 8 byte
	if (dataSize != 8) {
		std::cerr << "size of data in header must be 8 but " << dataSize << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// read index sizes (+ encode & additional UVs)
	for (auto i = 0; i < 8; ++i) read_Byte(indexSize_[i]);
}

void PMXLoader::readModelInfo() {
	// model name
	skip_TextBuf();
	// model name (en)
	skip_TextBuf();
	// comment
	skip_TextBuf();
	// comment (en)
	skip_TextBuf();
}

void PMXLoader::readVertexData(std::vector<PMX_Vertex>& vertices) {
	std::int32_t verticesCount{};
	read_Int(verticesCount);
	vertices.resize(verticesCount);

	//std::cout << "# of vertex: " << verticesCount << std::endl;

	for (auto i = 0; i < verticesCount; ++i) {
		// position
		read_Float3(vertices[i].position);
		// normal
		read_Float3(vertices[i].normal);
		// uv
		read_Float2(vertices[i].uv);

		// additional UVs (0 ~ 4)
		for (auto j = 0; j < getIndexSize(Index::ADDITIONAL_UV); ++j) read_Float4(vertices[i].additionalUV[j]);

		// bone index & weight
		std::uint8_t weightType{};
		read_Byte(weightType);

		switch (weightType) {
		// BDEF1
		case 0:
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[0]);
			vertices[i].boneIndices[1] = vertices[i].boneIndices[2] = vertices[i].boneIndices[3] = -1;
			break;
		// BDEF2
		case 1:
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[0]);
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[1]);
			vertices[i].boneIndices[2] = vertices[i].boneIndices[3] = -1;
			read_Float(vertices[i].boneWeights[0]);
			break;
		// BDEF4
		case 2:
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[0]);
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[1]);
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[2]);
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[3]);
			read_Float(vertices[i].boneWeights[0]);
			read_Float(vertices[i].boneWeights[1]);
			read_Float(vertices[i].boneWeights[2]);
			read_Float(vertices[i].boneWeights[3]);
			break;
		// SDEF (currently treated as BDEF2)
		case 3:
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[0]);
			read_Index(getIndexSize(Index::BONE), vertices[i].boneIndices[1]);
			vertices[i].boneIndices[2] = vertices[i].boneIndices[3] = -1;
			read_Float(vertices[i].boneWeights[0]);
			// SDEF-C
			skip_Float3();
			// SDEF-R0
			skip_Float3();
			// SDEF-R1
			skip_Float3();
			break;
		default:
			std::cerr << "illegal type of bone weights" << std::endl;
			std::exit(EXIT_FAILURE);
			break;
		}

		// edge multiplification
		read_Float(vertices[i].edgeMult);
	}
}

void PMXLoader::readFaceData(PMX_Indices& indices) {
	std::int32_t indicesCount{};
	read_Int(indicesCount);

	//std::cout << "# of index: " << indicesCount << std::endl;

	if (getIndexSize(Index::VERTEX) == 1 || getIndexSize(Index::VERTEX) == 2) {
		indices = std::vector<uint16_t>(indicesCount);
		for (auto i = 0; i < indicesCount; ++i) {
			std::int32_t index{};
			read_Index(getIndexSize(Index::VERTEX), index);
			std::get<std::vector<std::uint16_t>>(indices)[i] = index;
		}
	}
	else if (getIndexSize(Index::VERTEX) == 4) {
		indices = std::vector<uint32_t>(indicesCount);
		for (auto i = 0; i < indicesCount; ++i) {
			std::int32_t index{};
			read_Index(getIndexSize(Index::VERTEX), index);
			std::get<std::vector<std::uint32_t>>(indices)[i] = index;
		}
	}
	else {
		std::cerr << "illegal size of indices" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void PMXLoader::readTextureData(std::vector<PMX_TexturePath>& texturePaths) {
	std::int32_t pathsCount{};
	read_Int(pathsCount);
	texturePaths.resize(pathsCount);

	//std::cout << "# of texture: " << pathsCount << std::endl;

	// UTF16
	if (getIndexSize(Index::ENCODE) == 0) {
		for (auto i = 0; i < pathsCount; ++i) {
			std::vector<std::uint16_t> pathBytes{};
			read_TextBuf(pathBytes);
			texturePaths[i] = parentPath_ / std::filesystem::path(std::u16string(pathBytes.begin(), pathBytes.end()));
		}
	}
	// UTF8
	else if (getIndexSize(Index::ENCODE) == 1) {
		for (auto i = 0; i < pathsCount; ++i) {
			std::vector<std::uint8_t> pathBytes{};
			read_TextBuf(pathBytes);
			texturePaths[i] = parentPath_ / std::filesystem::path(std::u8string(pathBytes.begin(), pathBytes.end()));
		}
	}
	else {
		std::cerr << "illegal encode for texture paths" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void PMXLoader::readMaterialData(std::vector<PMX_Material>& materials) {
	std::int32_t materialsCount{};
	read_Int(materialsCount);
	materials.resize(materialsCount);

	//std::cout << "# of material: " << materialsCount << std::endl;

	// accumulate face index for offset
	std::int32_t indexOffset = 0;

	for (auto i = 0; i < materialsCount; ++i) {
		// name
		skip_TextBuf();
		// name (en)
		skip_TextBuf();

		// diffuse
		read_Float4(materials[i].diffuse);
		// specular
		read_Float3(materials[i].specular);
		// specular coefficient
		read_Float(materials[i].specCoef);
		// ambient
		read_Float3(materials[i].ambient);

		// flags
		read_Byte(materials[i].flags);

		// edge color
		read_Float4(materials[i].edgeColor);
		// edge size;
		read_Float(materials[i].edgeSize);

		// texture index
		read_Index(getIndexSize(Index::TEXTURE), materials[i].textureIndex);
		// sphere texture index
		read_Index(getIndexSize(Index::TEXTURE), materials[i].sphereIndex);
		// sphere mode
		read_Byte(materials[i].sphereMode);

		// is shared toon
		read_Byte(materials[i].isSharedToon);

		// shared toon index (byte)
		if (materials[i].isSharedToon) read_Index(1, materials[i].toonIndex);
		// unique toon index (texture index size)
		else read_Index(getIndexSize(Index::TEXTURE), materials[i].toonIndex);

		// memo
		skip_TextBuf();

		// face count
		std::int32_t count{};
		read_Int(count);
		materials[i].indexOffset = indexOffset;
		materials[i].indexCount = count;
		indexOffset += count;
	}
}

void PMXLoader::readBoneData(std::vector<PMX_Bone>& bones) {
	std::int32_t bonesCount{};
	read_Int(bonesCount);
	bones.resize(bonesCount);

	//std::cout << "# of bone: " << bonesCount << std::endl;

	for (auto i = 0; i < bonesCount; ++i) {
		bones[i].index = i;
		// name
		std::vector<std::uint16_t> name{}, name_en{};
		//skip_TextBuf();
		read_TextBuf(name);
		// name (en)
		//skip_TextBuf();
		read_TextBuf(name_en);

		std::cout << "bone #" << i << std::endl;
		std::cout << std::filesystem::path(std::u16string(name.begin(), name.end())) << std::endl;
		std::cout << std::filesystem::path(std::u16string(name_en.begin(), name_en.end())) << std::endl;

		// position
		read_Float3(bones[i].position);
		// parent index
		read_Index(getIndexSize(Index::BONE), bones[i].parentIndex);
		// hierarchy
		read_Int(bones[i].hierarchy);

		// flags
		read_uShort(bones[i].flags);

		// connection
		// bone
		if (bones[i].flags & 0x0001) skip_Index(getIndexSize(Index::BONE));
		// offset
		else skip_Float3();

		// give
		if ((bones[i].flags & 0x0100) || (bones[i].flags & 0x0200)) {
			// bone index
			skip_Index(getIndexSize(Index::BONE));
			// rate
			skip_Float();
		}

		// fixed axis
		if (bones[i].flags & 0x0400) skip_Float3();

		// local axis
		if (bones[i].flags & 0x0800) {
			// X axis
			skip_Float3();
			// Z axis
			skip_Float3();
		}

		// external parent
		if (bones[i].flags & 0x2000) skip_Int();

		// IK
		if (bones[i].flags & 0x0020) {
			// target index
			read_Index(getIndexSize(Index::BONE), bones[i].ik.targetIndex);
			// loop count
			read_Int(bones[i].ik.loopCount);
			// limit
			read_Float(bones[i].ik.limit_rad);

			// links count
			std::int32_t linksCount{};
			read_Int(linksCount);
			bones[i].ik.links.resize(linksCount);

			for (auto j = 0; j < linksCount; ++j) {
				// link bone index
				read_Index(getIndexSize(Index::BONE), bones[i].ik.links[j].index);
				// is limited
				read_Byte(bones[i].ik.links[j].isLimited);

				if (bones[i].ik.links[j].isLimited) {
					// lower bound
					read_Float3(bones[i].ik.links[j].lowerBound_rad);
					// upper bound
					read_Float3(bones[i].ik.links[j].upperBound_rad);
				}
			}
		}
		else {
			bones[i].ik.targetIndex = -1;
		}
	}
}

void PMXLoader::readMorphData(std::vector<PMX_Morph>& morphs) {
	std::int32_t morphsCount{};
	read_Int(morphsCount);
	morphs.resize(morphsCount);

	// std::cout << "# of morph: " << morphsCount << std::endl;
	
	for (auto i = 0; i < morphsCount; ++i) {
		// name
		skip_TextBuf();
		// name (en)
		skip_TextBuf();

		// pane
		skip_Byte();

		// type
		std::uint8_t morphType{};
		read_Byte(morphType);

		// count
		std::int32_t offsetsCount{};
		read_Int(offsetsCount);

		switch (morphType) {
		// group
		case PMX_Morph_Type::GROUP:
			morphs[i] = std::vector<PMX_Morph_Group>(offsetsCount);
			for (auto j = 0; j < offsetsCount; ++j) {
				// morph index
				std::int32_t index{};
				read_Index(getIndexSize(Index::MORPH), index);
				// rate
				glm::float32_t rate{};
				read_Float(rate);

				std::get<PMX_Morph_Type::GROUP>(morphs[i])[j] = PMX_Morph_Group{ index, rate };
			}
			break;
		// vertex
		case PMX_Morph_Type::VERTEX:
			morphs[i] = std::vector<PMX_Morph_Vertex>(offsetsCount);
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::VERTEX), index);
				// offset
				glm::vec3 offset{};
				read_Float3(offset);

				std::get<PMX_Morph_Type::VERTEX>(morphs[i])[j] = PMX_Morph_Vertex{ index, offset };
			}
			break;
		// bone
		case PMX_Morph_Type::BONE:
			morphs[i] = std::vector<PMX_Morph_Bone>(offsetsCount);
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::BONE), index);
				// translate offset
				glm::vec3 transOffset{};
				read_Float3(transOffset);
				// rotate offset
				glm::vec4 rotOffset_quat{};
				read_Float4(rotOffset_quat);

				std::get<PMX_Morph_Type::BONE>(morphs[i])[j] = PMX_Morph_Bone{ index, transOffset, rotOffset_quat };
			}
			break;
		// UV
		case PMX_Morph_Type::UV:
			morphs[i] = PMX_Morph{ std::in_place_index<PMX_Morph_Type::UV>, std::vector<PMX_Morph_UV>(offsetsCount) };
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::VERTEX), index);
				// offset
				glm::vec4 offset{};
				read_Float4(offset);

				std::get<PMX_Morph_Type::UV>(morphs[i])[j] = PMX_Morph_UV{ index, offset };
			}
			break;
		// additional UV 1
		case PMX_Morph_Type::ADD_UV1:
			morphs[i] = PMX_Morph{ std::in_place_index<PMX_Morph_Type::ADD_UV1>, std::vector<PMX_Morph_UV>(offsetsCount) };
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::VERTEX), index);
				// offset
				glm::vec4 offset{};
				read_Float4(offset);

				std::get<PMX_Morph_Type::ADD_UV1>(morphs[i])[j] = PMX_Morph_UV{ index, offset };
			}
			break;
		// additional UV 2
		case PMX_Morph_Type::ADD_UV2:
			morphs[i] = PMX_Morph{ std::in_place_index<PMX_Morph_Type::ADD_UV2>, std::vector<PMX_Morph_UV>(offsetsCount) };
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::VERTEX), index);
				// offset
				glm::vec4 offset{};
				read_Float4(offset);

				std::get<PMX_Morph_Type::ADD_UV2>(morphs[i])[j] = PMX_Morph_UV{ index, offset };
			}
			break;
		// additional UV 3
		case PMX_Morph_Type::ADD_UV3:
			morphs[i] = PMX_Morph{ std::in_place_index<PMX_Morph_Type::ADD_UV3>, std::vector<PMX_Morph_UV>(offsetsCount) };
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::VERTEX), index);
				// offset
				glm::vec4 offset{};
				read_Float4(offset);

				std::get<PMX_Morph_Type::ADD_UV3>(morphs[i])[j] = PMX_Morph_UV{ index, offset };
			}
			break;
		// additional UV 4
		case PMX_Morph_Type::ADD_UV4:
			morphs[i] = PMX_Morph{ std::in_place_index<PMX_Morph_Type::ADD_UV4>, std::vector<PMX_Morph_UV>(offsetsCount) };
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::VERTEX), index);
				// offset
				glm::vec4 offset{};
				read_Float4(offset);

				std::get<PMX_Morph_Type::ADD_UV4>(morphs[i])[j] = PMX_Morph_UV{ index, offset };
			}
			break;
		// material
		case PMX_Morph_Type::MATERIAL:
			morphs[i] = std::vector<PMX_Morph_Group>(offsetsCount);
			for (auto j = 0; j < offsetsCount; ++j) {
				// index
				std::int32_t index{};
				read_Index(getIndexSize(Index::MATERIAL), index);
				// calculation mode
				std::uint8_t calcMode{};
				read_Byte(calcMode);
				// diffuse
				glm::vec4 diffuse{};
				read_Float4(diffuse);
				// specular
				glm::vec3 specular{};
				read_Float3(specular);
				// specular coefficient
				glm::float32_t specCoef{};
				read_Float(specCoef);
				// ambient
				glm::vec3 ambient{};
				read_Float3(ambient);
				// edge color
				glm::vec4 edgeColor{};
				read_Float4(edgeColor);
				// edge size
				glm::float32_t edgeSize{};
				read_Float(edgeSize);
				// texture coefficient
				glm::vec4 textureCoef{};
				read_Float4(textureCoef);
				// sphere texture coefficient
				glm::vec4 sphereCoef{};
				read_Float4(sphereCoef);
				// toon texture coefficient
				glm::vec4 toonCoef{};
				read_Float4(toonCoef);

				std::get<PMX_Morph_Type::MATERIAL>(morphs[i])[j] = PMX_Morph_Material{ index, calcMode, diffuse, specular, specCoef, ambient, edgeColor, edgeSize, textureCoef, sphereCoef, toonCoef };
			}
			break;

		default:
			std::cerr << "invalid morph type" << std::endl;
			std::exit(EXIT_FAILURE);
			break;
		}
	}
}

void PMXLoader::readFrameData() {
	std::int32_t framesCount{};
	read_Int(framesCount);

	//std::cout << "# of frame: " << framesCount << std::endl;

	for (auto i = 0; i < framesCount; ++i) {
		// name
		skip_TextBuf();
		// name (en)
		skip_TextBuf();

		// flag
		skip_Byte();

		// elements count
		std::int32_t count{};
		read_Int(count);

		for (auto j = 0; j < count; ++j) {
			// target type
			std::uint8_t type{};
			read_Byte(type);

			// morph
			if (type) skip_Index(getIndexSize(Index::MORPH));
			// bone
			else skip_Index(getIndexSize(Index::BONE));
		}
	}
}

void PMXLoader::readRigidData(std::vector<PMX_Rigid>& rigids) {
	std::int32_t rigidsCount{};
	read_Int(rigidsCount);
	rigids.resize(rigidsCount);

	//std::cout << "# of rigid: " << rigidsCount << std::endl;

	for (auto i = 0; i < rigidsCount; ++i) {
		// name
		skip_TextBuf();
		// name (en)
		skip_TextBuf();

		// bone index
		read_Index(getIndexSize(Index::BONE), rigids[i].index);

		// group
		read_Byte(rigids[i].group);
		// group flag
		read_uShort(rigids[i].groupFlag);

		// shape
		read_Byte(rigids[i].shape);
		// size
		read_Float3(rigids[i].size);

		// position
		read_Float3(rigids[i].position);
		// rotation
		read_Float3(rigids[i].rotate_rad);

		// mass
		read_Float(rigids[i].mass);
		// translate attenuation
		read_Float(rigids[i].transAtte);
		// rotate attenuation
		read_Float(rigids[i].rotAtte);
		// reflection
		read_Float(rigids[i].reflection);
		// friction
		read_Float(rigids[i].friction);

		// calculation mode
		read_Byte(rigids[i].calcMode);
	}
}

void PMXLoader::readJointData(std::vector<PMX_Joint>& joints) {
	std::int32_t jointsCount{};
	read_Int(jointsCount);
	joints.resize(jointsCount);

	//std::cout << "# of joint: " << jointsCount << std::endl;

	for (auto i = 0; i < jointsCount; ++i) {
		// name
		skip_TextBuf();
		// name (en)
		skip_TextBuf();

		// type (0 only)
		skip_Byte();

		// rigid index A
		read_Index(getIndexSize(Index::RIGID), joints[i].indexA);
		// rigid index B
		read_Index(getIndexSize(Index::RIGID), joints[i].indexB);

		// position
		read_Float3(joints[i].position);
		// rotation
		read_Float3(joints[i].rotate_rad);

		// translate lower bound
		read_Float3(joints[i].transLower);
		// translate upper bound
		read_Float3(joints[i].transUpper);
		// rotate lower bound
		read_Float3(joints[i].rotLower_rad);
		// rotate upper bound
		read_Float3(joints[i].rotUpper_rad);

		// spring constant translate
		read_Float3(joints[i].springTrans);
		// spring constant rotate
		read_Float3(joints[i].springRot);
	}
}

void PMXLoader::load(const std::filesystem::path& path, PMXData& data) {

	ifs_ = std::ifstream(path, std::ios::in | std::ios::binary);
	if (ifs_.fail()) {
		std::cerr << "failed to open PMX file: " << path << std::endl;
		std::exit(EXIT_FAILURE);
	}

	parentPath_ = path.parent_path();

	readHeader();

	readModelInfo();

	readVertexData(data.vertices);

	readFaceData(data.indices);

	readTextureData(data.texturePaths);

	readMaterialData(data.materials);

	readBoneData(data.bones);

	readMorphData(data.morphs);

	readFrameData();

	readRigidData(data.rigids);

	readJointData(data.joints);

	ifs_.close();
}