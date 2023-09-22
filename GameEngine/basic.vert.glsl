#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec4 vColor;

layout(binding = 0) uniform UniformBufferObject{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(push_constant) uniform PushConstants{
	mat4 model;
	mat4 view;
	mat4 projection;
} pc;

void main() {
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0f);
	vNormal = (ubo.model * vec4(normal, 0.0f)).xyz;
	vColor = color;
}