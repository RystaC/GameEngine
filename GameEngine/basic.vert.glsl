#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 a_uv1;
layout(location = 4) in vec4 a_uv2;
layout(location = 5) in vec4 a_uv3;
layout(location = 6) in vec4 a_uv4;

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

void main() {
	vec3 light = vec3(-5.0f, 5.0f, -5.0f);

	gl_Position = transform.projection * transform.view * transform.model * vec4(position, 1.0f);
	viewPosition = vec3(transform.view * transform.model * vec4(position, 1.0f));
	viewNormal = vec3(transform.view * transform.normalMatrix * vec4(normal, 0.0f));
	vTexCoord = uv;

	viewLight = vec3(transform.view * vec4(light, 1.0f));
}