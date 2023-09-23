#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 color;

layout(location = 0) out vec3 viewPosition;
layout(location = 1) out vec3 viewNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec4 vColor;
layout(location = 4) out vec3 viewLight;

layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 normalMatrix;
} ubo;

void main() {
	vec3 light = vec3(5.0f);
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0f);
	viewPosition = vec3(ubo.view * ubo.model * vec4(position, 1.0f));
	viewNormal = vec3(ubo.view * ubo.normalMatrix * vec4(normal, 0.0f));
	vTexCoord = texCoord;
	vColor = color;

	viewLight = vec3(ubo.view * vec4(light, 1.0f));
}