#version 460

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 color;

void main() {
	vec3 parallelLight = normalize(vec3(-1.0f, -1.0f, 1.0f));

	float diffuse = clamp(dot(vNormal, parallelLight), 0.1f, 1.0f);

	color = vec4(vec3(diffuse), 1.0f) * vColor;
}