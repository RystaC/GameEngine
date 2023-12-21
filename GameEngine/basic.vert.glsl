#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 a_uv1;
layout(location = 4) in vec4 a_uv2;
layout(location = 5) in vec4 a_uv3;
layout(location = 6) in vec4 a_uv4;
layout(location = 7) in ivec4 boneIndices;
layout(location = 8) in vec4 boneWeights;
layout(location = 9) in float edgeMult;

layout(location = 0) out vec3 viewPosition;
layout(location = 1) out vec3 viewNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec3 viewLight;

layout(binding = 0) uniform TransformBufferObject{
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 normalMatrix;
} transform;

layout(std430, binding = 5) buffer BoneMatrix {
	mat4 boneMatrix[];
};

void main() {
	vec3 light = vec3(-5.0f, 5.0f, -5.0f);

	vec4 pos = vec4(position, 1.0f);
	vec4 skinnedPos;
	vec4 nor = vec4(normal, 0.0f);
	vec4 skinnedNor;

	// BDEF1
	if (boneIndices.y == -1) {
		skinnedPos = (boneMatrix[boneIndices.x] * pos);
		skinnedNor = (boneMatrix[boneIndices.x] * nor);
	}
	// BDEF2 (or SDEF)
	else if (boneIndices.z == -1) {
		skinnedPos = (boneMatrix[boneIndices.x] * pos) * boneWeights.x;
		skinnedPos += (boneMatrix[boneIndices.y] * pos) * (1.0f - boneWeights.x);
		skinnedNor = (boneMatrix[boneIndices.x] * nor) * boneWeights.x;
		skinnedNor += (boneMatrix[boneIndices.y] * nor) * (1.0f - boneWeights.x);
	}
	// BDEF4
	else {
		skinnedPos = (boneMatrix[boneIndices.x] * pos) * boneWeights.x;
		skinnedPos += (boneMatrix[boneIndices.y] * pos) * boneWeights.y;
		skinnedPos += (boneMatrix[boneIndices.z] * pos) * boneWeights.z;
		skinnedPos += (boneMatrix[boneIndices.w] * pos) * boneWeights.w;
		skinnedNor = (boneMatrix[boneIndices.x] * nor) * boneWeights.x;
		skinnedNor += (boneMatrix[boneIndices.y] * nor) * boneWeights.y;
		skinnedNor += (boneMatrix[boneIndices.z] * nor) * boneWeights.z;
		skinnedNor += (boneMatrix[boneIndices.w] * nor) * boneWeights.w;
	}

	//gl_Position = transform.projection * transform.view * transform.model * vec4(position, 1.0f);
	gl_Position = transform.projection * transform.view * transform.model * skinnedPos;
	//viewPosition = vec3(transform.view * transform.model * vec4(position, 1.0f));
	viewPosition = vec3(transform.view * transform.model * skinnedPos);
	//viewNormal = vec3(transform.view * transform.normalMatrix * vec4(normal, 0.0f));
	viewNormal = vec3(transform.view * transform.normalMatrix * skinnedNor);
	vTexCoord = uv;

	viewLight = vec3(transform.view * vec4(light, 1.0f));
}