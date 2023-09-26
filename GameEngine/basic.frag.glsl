#version 460

layout(location = 0) in vec3 viewPosition;
layout(location = 1) in vec3 viewNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec4 vColor;
layout(location = 4) in vec3 viewLight;

layout(location = 0) out vec4 color;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
	vec3 N = normalize(viewNormal);
	vec3 L = normalize(viewLight - viewPosition);
	vec3 V = normalize(-viewPosition);
	vec3 H = normalize(L + V);

	float ambient = 0.1f;
	float diffuse = clamp(dot(L, N), 0.0f, 1.0f);
	float specular = pow(clamp(dot(H, N), 0.0f, 1.0f), 64.0f);

	vec3 colorComponent = texture(texSampler, vTexCoord).xyz + vColor.xyz;
	float alphaComponent = 1.0f;

	color = vec4(ambient * colorComponent + diffuse * colorComponent + specular * vec3(1.0f), alphaComponent);
}