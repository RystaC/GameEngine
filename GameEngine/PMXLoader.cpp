#include "PMXLoader.h"

void PMXLoader::readHeader() {
	Byte magic[4]{};
	ifs.read((char*)magic, sizeof(Byte) * 4);

	if (!(std::string_view((const char*)magic, 4) == "PMX ")) {
		std::cerr << "Input file is not a PMX file" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	Float version{};
	ifs.read((char*)&version, sizeof(Float));
	std::cout << "PMX version: " << version << std::endl;

	Byte infoSize{};
	ifs.read((char*)&infoSize, sizeof(infoSize));
	std::cout << "information block size (must be 8): " << (int)infoSize << std::endl;

	ifs.read((char*)info, sizeof(Byte) * 8);
	Encode encode = getIndexSize(IndexSize::ENCODE) == 0 ? Encode::UTF16LE : Encode::UTF8;
	if (encode == Encode::UTF16LE) std::cout << "text encode is UTF16LE" << std::endl;
	else std::cout << "text encode is UTF8" << std::endl;
	std::cout << "# of additional UVs: " << (int)getIndexSize(IndexSize::ADDITIONAL_UV) << std::endl;
	std::cout << "vertex index size: " << (int)getIndexSize(IndexSize::VERTEX) << std::endl;
	std::cout << "texture index size: " << (int)getIndexSize(IndexSize::TEXTURE) << std::endl;
	std::cout << "material index size: " << (int)getIndexSize(IndexSize::MATERIAL) << std::endl;
	std::cout << "bone index size: " << (int)getIndexSize(IndexSize::BONE) << std::endl;
	std::cout << "morph index size: " << (int)getIndexSize(IndexSize::MORPH) << std::endl;
	std::cout << "rigid index size: " << (int)getIndexSize(IndexSize::RIGID) << std::endl;
}

void PMXLoader::readModelInfo() {
	ModelInfo modelInfo{};
	readTextBuf(modelInfo.name);
	readTextBuf(modelInfo.name_en);
	readTextBuf(modelInfo.comment);
	readTextBuf(modelInfo.comment_en);

	std::cout << "model name: " << UTF16_TEXT(modelInfo.name) << std::endl;
	std::cout << "model name (en):" << UTF16_TEXT(modelInfo.name_en) << std::endl;
	std::cout << "comment: " << UTF16_TEXT(modelInfo.comment) << std::endl;
	std::cout << "comment (en): " << UTF16_TEXT(modelInfo.comment_en) << std::endl;
}

void PMXLoader::readVertexData() {
	Int numVertices{};
	ifs.read((char*)&numVertices, sizeof(Int));
	std::cout << "# of vertices: " << numVertices << std::endl;

	struct {
		int bdef1, bdef2, bdef4, sdef;
	} weightCount{};

	for (auto i = 0; i < numVertices; ++i) {
		Attribute attr{};
		ifs.read((char*)&attr, sizeof(Attribute));

		for (auto numAdds = 0; numAdds < getIndexSize(IndexSize::ADDITIONAL_UV); ++numAdds) {
			Float4 addUv{};
			ifs.read((char*)&addUv, sizeof(Float4));
		}

		Byte type{};
		ifs.read((char*)&type, sizeof(Byte));

		BoneWeight boneWeight{};

		switch ((WeightType)type) {
		case WeightType::BDEF1:
			boneWeight.type = (WeightType)type;
			readBDEF1(boneWeight.weight.bdef1);
			weightCount.bdef1 += 1;
			break;
		case WeightType::BDEF2:
			boneWeight.type = (WeightType)type;
			readBDEF2(boneWeight.weight.bdef2);
			weightCount.bdef2 += 1;
			break;
		case WeightType::BDEF4:
			boneWeight.type = (WeightType)type;
			readBDEF4(boneWeight.weight.bdef4);
			weightCount.bdef4 += 1;
			break;
		case WeightType::SDEF:
			boneWeight.type = (WeightType)type;
			readSDEF(boneWeight.weight.sdef);
			weightCount.sdef += 1;
			break;
		default:
			break;
		}

		Float edgeMag{};
		ifs.read((char*)&edgeMag, sizeof(Float));
	}

	std::cout << "vertex data statistics:" << std::endl;
	std::cout << "\t# of BDEF1: " << weightCount.bdef1 << std::endl;
	std::cout << "\t# of BDEF2: " << weightCount.bdef2 << std::endl;
	std::cout << "\t# of BDEF4: " << weightCount.bdef4 << std::endl;
	std::cout << "\t# of SDEF: " << weightCount.sdef << std::endl;
}

void PMXLoader::readFaceData() {
	Int numIndices{};
	ifs.read((char*)&numIndices, sizeof(Int));
	std::cout << "# of indices: " << numIndices << std::endl;

	for (auto i = 0; i < numIndices; ++i) {
		Int index{};
		ifs.read((char*)&index, getIndexSize(IndexSize::VERTEX));
	}
}

void PMXLoader::readTextureData() {
	Int numTextures{};
	ifs.read((char*)&numTextures, sizeof(Int));
	std::cout << "# of textures: " << numTextures << std::endl;

	for (auto i = 0; i < numTextures; ++i) {
		TextBuf texturePath{};
		readTextBuf(texturePath);
		std::cout << "\ttexture #" << i << ": " << UTF16_TEXT(texturePath) << std::endl;
	}
}

void PMXLoader::readMaterialData() {
	Int numMaterials{};
	ifs.read((char*)&numMaterials, sizeof(Int));
	std::cout << "# of materials: " << numMaterials << std::endl;

	for (auto i = 0; i < numMaterials; ++i) {
		TextBuf name{}, name_en{};
		readTextBuf(name);
		readTextBuf(name_en);
		std::cout << "material #" << i << ":" << std::endl;
		std::cout << "\tname: " << UTF16_TEXT(name) << std::endl;
		std::cout << "\tname (en): " << UTF16_TEXT(name_en) << std::endl;

		MaterialColor materialColor{};
		ifs.read((char*)&materialColor, sizeof(MaterialColor));

		BitFlag flag{};
		ifs.read((char*)&flag, sizeof(BitFlag));

		Float4 edgeColor{};
		Float edgeSize{};
		ifs.read((char*)&edgeColor, sizeof(Float4));
		ifs.read((char*)&edgeSize, sizeof(Float));

		Int texIndex{}, sphereIndex{};
		ifs.read((char*)&texIndex, getIndexSize(IndexSize::TEXTURE));
		ifs.read((char*)&sphereIndex, getIndexSize(IndexSize::TEXTURE));
		std::cout << "\ttexture index: " << texIndex << std::endl;
		std::cout << "\tsphere index: " << sphereIndex << std::endl;

		Byte sphereMode{}, toonFlag{};
		ifs.read((char*)&sphereMode, sizeof(Byte));
		ifs.read((char*)&toonFlag, sizeof(Byte));

		if (toonFlag == 0) {
			Int toonIndex{};
			ifs.read((char*)&toonIndex, getIndexSize(IndexSize::TEXTURE));
			std::cout << "\ttoon texture index (unique): " << toonIndex << std::endl;
		}
		else {
			Byte sharedIndex{};
			ifs.read((char*)&sharedIndex, sizeof(Byte));
			std::cout << "\ttoon texture index (shared): " << (int)sharedIndex << std::endl;
		}

		TextBuf memo{};
		readTextBuf(memo);
		std::cout << "\tmemo: " << UTF16_TEXT(memo) << std::endl;

		Int numFaces{};
		ifs.read((char*)&numFaces, sizeof(Int));
		std::cout << "\t# of faces: " << numFaces << std::endl;
	}
}

void PMXLoader::readBoneData() {
	Int numBones{};
	ifs.read((char*)&numBones, sizeof(Int));
	std::cout << "# of bones: " << numBones << std::endl;

	for (auto i = 0; i < numBones; ++i) {
		TextBuf name{}, name_en{};
		readTextBuf(name);
		readTextBuf(name_en);
		std::cout << "bone #" << i << ":" << std::endl;
		std::cout << "\tname: " << UTF16_TEXT(name) << std::endl;
		std::cout << "\tname (en): " << UTF16_TEXT(name_en) << std::endl;

		Float3 position{};
		Int parentIndex{};
		Int hierarchy{};
		ifs.read((char*)&position, sizeof(Float3));
		ifs.read((char*)&parentIndex, getIndexSize(IndexSize::BONE));
		ifs.read((char*)&hierarchy, sizeof(Int));

		// BitFlag * 2
		uShort boneFlag{};
		ifs.read((char*)&boneFlag, sizeof(uShort));

		constexpr uShort connectMask = 0x0001;
		constexpr uShort isIk = 0x0020;

		constexpr uShort globalGive = 0x0100 | 0x0200;
		constexpr uShort axisFix = 0x0400;
		constexpr uShort localAxis = 0x0800;
		constexpr uShort extTrans = 0x2000;

		if (!(boneFlag & connectMask)) {
			Float3 offset{};
			ifs.read((char*)&offset, sizeof(Float3));
		}
		else {
			Int boneIndex{};
			ifs.read((char*)&boneIndex, getIndexSize(IndexSize::BONE));
		}

		if (boneFlag & globalGive) {
			Int boneIndex{};
			Float rate{};
			ifs.read((char*)&boneIndex, getIndexSize(IndexSize::BONE));
			ifs.read((char*)&rate, sizeof(Float));
		}

		if (boneFlag & axisFix) {
			Float3 axisVector{};
			ifs.read((char*)&axisVector, sizeof(Float3));
		}

		if (boneFlag & localAxis) {
			Float3 xAxis{}, zAxis{};
			ifs.read((char*)&xAxis, sizeof(Float3));
			ifs.read((char*)&zAxis, sizeof(Float3));
		}

		if (boneFlag & extTrans) {
			Int key{};
			ifs.read((char*)&key, sizeof(Int));
		}

		if (boneFlag & isIk) {
			Int targetIndex{};
			ifs.read((char*)&targetIndex, getIndexSize(IndexSize::BONE));

			Int numLoops{};
			Float limitRadian{};
			ifs.read((char*)&numLoops, sizeof(Int));
			ifs.read((char*)&limitRadian, sizeof(Float));

			Int numLinks{};
			ifs.read((char*)&numLinks, sizeof(Int));

			for (auto l = 0; l < numLinks; ++l) {
				Int linkIndex{};
				ifs.read((char*)&linkIndex, getIndexSize(IndexSize::BONE));

				Byte isLimited{};
				ifs.read((char*)&isLimited, sizeof(Byte));

				if (isLimited) {
					Float3 lower{}, upper{};
					ifs.read((char*)&lower, sizeof(Float3));
					ifs.read((char*)&upper, sizeof(Float3));
				}
			}
		}
	}
}

void PMXLoader::readMorphData() {
	Int numMorphs{};
	ifs.read((char*)&numMorphs, sizeof(Int));
	std::cout << "# of morphs: " << numMorphs << std::endl;

	for (auto i = 0; i < numMorphs; ++i) {
		std::cout << "morph #" << i << ":" << std::endl;

		TextBuf name{}, name_en{};
		readTextBuf(name);
		readTextBuf(name_en);

		std::cout << "\t name: " << UTF16_TEXT(name) << std::endl;
		std::cout << "\t name (en): " << UTF16_TEXT(name_en) << std::endl;

		Byte ctrlPane{}, type{};
		ifs.read((char*)&ctrlPane, sizeof(Byte));
		ifs.read((char*)&type, sizeof(Byte));

		Int numOffsets{};
		ifs.read((char*)&numOffsets, sizeof(Int));

		for (auto o = 0; o < numOffsets; ++o) {
			Byte calcType{};

			Morph morph{};

			switch ((MorphType)type) {
			case MorphType::GROUP:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::MORPH));
				ifs.read((char*)&morph.data.group.rate, sizeof(Float));
				break;
			case MorphType::VERTEX:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::VERTEX));
				ifs.read((char*)&morph.data.vertex.offset, sizeof(Float3));
				break;
			case MorphType::BONE:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::BONE));
				ifs.read((char*)&morph.data.bone.move, sizeof(Float3));
				ifs.read((char*)&morph.data.bone.rotate_quat, sizeof(Float4));
				break;
			case MorphType::UV:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::VERTEX));
				ifs.read((char*)&morph.data.uv.offset, sizeof(Float4));
				break;
			case MorphType::A_UV1:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::VERTEX));
				ifs.read((char*)&morph.data.a_uv1.offset, sizeof(Float4));
				break;
			case MorphType::A_UV2:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::VERTEX));
				ifs.read((char*)&morph.data.a_uv2.offset, sizeof(Float4));
				break;
			case MorphType::A_UV3:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::VERTEX));
				ifs.read((char*)&morph.data.a_uv3.offset, sizeof(Float4));
				break;
			case MorphType::A_UV4:
				morph.type = (MorphType)type;
				ifs.read((char*)&morph.index, getIndexSize(IndexSize::VERTEX));
				ifs.read((char*)&morph.data.a_uv4.offset, sizeof(Float4));
				break;
			case MorphType::MATERIAL:
				ifs.read((char*)&morph.index,  getIndexSize(IndexSize::MATERIAL));
				ifs.read((char*)&calcType, sizeof(Byte));
				if (calcType == 0) {
					morph.type = MorphType::MATERIAL_MUL;
					ifs.read((char*)&morph.data.material_mul, sizeof(MaterialMorph));
				}
				else {
					morph.type = MorphType::MATERIAL_ADD;
					ifs.read((char*)&morph.data.material_add, sizeof(MaterialMorph));
				}
				break;
			default:
				break;
			}
		}
	}
}

void PMXLoader::readFrameData() {
	Int numFrames{};
	ifs.read((char*)&numFrames, sizeof(Int));
	std::cout << "# of frames: " << numFrames << std::endl;

	for (auto i = 0; i < numFrames; ++i) {
		std::cout << "frame #" << i << ":" << std::endl;

		TextBuf name{}, name_en{};
		readTextBuf(name);
		readTextBuf(name_en);
		std::cout << "\tname: " << UTF16_TEXT(name) << std::endl;
		std::cout << "\tname (en): " << UTF16_TEXT(name_en) << std::endl;

		Byte frameFlag{};
		ifs.read((char*)&frameFlag, sizeof(Byte));

		Int numElements{};
		ifs.read((char*)&numElements, sizeof(Int));

		for (auto e = 0; e < numElements; ++e) {
			Byte target{};
			ifs.read((char*)&target, sizeof(Byte));

			FrameElement frameElement{};

			if (target == 0) {
				frameElement.target = FrameTarget::BONE;
				ifs.read((char*)&frameElement.index.bone, getIndexSize(IndexSize::BONE));
			}
			else {
				frameElement.target = FrameTarget::MORPH;
				ifs.read((char*)&frameElement.index.morph, getIndexSize(IndexSize::MORPH));
			}
		}
	}
}

void PMXLoader::readRigidData() {
	Int numRigids{};
	ifs.read((char*)&numRigids, sizeof(Int));
	std::cout << "# of rigids: " << numRigids << std::endl;

	for (auto i = 0; i < numRigids; ++i) {
		std::cout << "rigid #" << i << std::endl;

		TextBuf name{}, name_en{};
		readTextBuf(name);
		readTextBuf(name_en);
		std::cout << "\tname: " << UTF16_TEXT(name) << std::endl;
		std::cout << "\tname (en): " << UTF16_TEXT(name_en) << std::endl;

		Int boneIndex{};
		ifs.read((char*)&boneIndex, getIndexSize(IndexSize::BONE));

		Byte group{};
		uShort groupFlag{};
		ifs.read((char*)&group, sizeof(Byte));
		ifs.read((char*)&groupFlag, sizeof(uShort));

		Byte shape{};
		Float3 size{};
		ifs.read((char*)&shape, sizeof(Byte));
		ifs.read((char*)&size, sizeof(Float3));

		struct RigidParams {
			Float3 position;
			Float3 rotateRadian;
			Float mass;
			Float transAtte;
			Float rotateAtte;
			Float repulsion;
			Float friction;
		} rigidParams{};
		ifs.read((char*)&rigidParams, sizeof(RigidParams));

		Byte calcType{};
		ifs.read((char*)&calcType, sizeof(Byte));
	}
}

void PMXLoader::readJointData() {
	Int numJoints{};
	ifs.read((char*)&numJoints, sizeof(Int));
	std::cout << "# of joints: " << numJoints << std::endl;

	for (auto i = 0; i < numJoints; ++i) {
		std::cout << "joint #" << i << ":" << std::endl;

		TextBuf name{}, name_en{};
		readTextBuf(name);
		readTextBuf(name_en);
		std::cout << "\tname: " << UTF16_TEXT(name) << std::endl;
		std::cout << "\tname (en): " << UTF16_TEXT(name_en) << std::endl;

		Byte jointType{};
		ifs.read((char*)&jointType, sizeof(Byte));

		Int rigidIndexA{}, rigidIndexB{};
		ifs.read((char*)&rigidIndexA, getIndexSize(IndexSize::RIGID));
		ifs.read((char*)&rigidIndexB, getIndexSize(IndexSize::RIGID));

		struct JointParams {
			Float3 position;
			Float3 rotateRadian;
			Float3 transLower, transUpper;
			Float3 rotateLowerRadian, rotateUpperRadian;
			Float3 springRateTrans;
			Float3 springRateRotate;
		} jointParams{};

		ifs.read((char*)&jointParams, sizeof(JointParams));
	}
}

void PMXLoader::load(const std::filesystem::path& path) {
	ifs = std::ifstream(path, std::ios::in | std::ios::binary);
	if (ifs.fail()) {
		std::cerr << "failed to open PMX file: " << path << std::endl;
		std::exit(EXIT_FAILURE);
	}

	readHeader();

	readModelInfo();

	readVertexData();

	readFaceData();

	readTextureData();

	readMaterialData();

	readBoneData();

	readMorphData();

	readFrameData();

	readRigidData();

	readJointData();

	ifs.close();
}