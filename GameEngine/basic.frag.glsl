#version 460

layout(location = 0) in vec3 viewPosition;
layout(location = 1) in vec3 viewNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec3 viewLight;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform MaterialBufferObject {
	vec4 diffuse;
	vec3 specular;
	float specCoef;
	vec3 ambient;
	bool isTextureUsed;
	bool isSphereUsed;
	bool isToonUsed;
} material;

layout(binding = 2) uniform sampler2D textureSampler;
layout(binding = 3) uniform sampler2D sphereSampler;
layout(binding = 4) uniform sampler2D toonSampler;

void main() {
	vec3 N = normalize(viewNormal);
	vec3 L = normalize(viewLight - viewPosition);
	vec3 V = normalize(-viewPosition);
	vec3 H = normalize(L + V);

	float diffIntense = clamp(dot(N, L), 0.0f, 1.0f);
	float specIntense = pow(clamp(dot(H, N), 0.0f, 1.0f), material.specCoef);

	outColor = vec4(1.0f);

	/*outColor.rgb = material.ambient;
	if (!material.isToonUsed) {
		outColor.rgb += diffIntense * material.diffuse.rgb;
	}
	outColor.a = material.diffuse.a;
	outColor = clamp(outColor, 0.0f, 1.0f);*/
	
	if (material.isTextureUsed) {
		outColor *= texture(textureSampler, vTexCoord);
	}

	if (material.isSphereUsed) {
		vec2 sphereTexCoord = vec2(viewNormal.x * 0.5f + 0.5f, viewNormal.y * -0.5f + 0.5f);
		outColor.rgb += texture(sphereSampler, sphereTexCoord).rgb;
	}

	if (material.isToonUsed) {
		outColor.rgb *= texture(toonSampler, vec2(0.5f, 1.0f - diffIntense)).rgb;
	}
}