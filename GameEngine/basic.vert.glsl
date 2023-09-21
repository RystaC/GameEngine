#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 vColor;

layout(push_constant) uniform PushConstants{
	mat4 model;
	mat4 view;
	mat4 projection;
} pc;

void main() {
	gl_Position = pc.projection * pc.view * pc.model * vec4(position, 1.0f);
	vColor = color;
}