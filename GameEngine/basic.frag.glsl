#version 460

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 color;

void main() {
	color = vColor;
}