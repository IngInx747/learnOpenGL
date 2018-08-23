#version 330 core

/** Directional Light */

struct Directional_Light_t {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

vec3 CalcDirectionalLight(Directional_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular, sampler2D emission);

/** Point Light */

struct  Point_Light_t {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

vec3 CalcPointLight(Point_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular,
	vec4 fragPosLightSpace, sampler2D shadowMap);

/** Spot Light */

struct Spot_Light_t {
	vec3 position;
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
	float innerCutOff;
	float outerCutOff;
};

vec3 CalcSpotLight(Spot_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular);

/** Texture mapping */

struct MatTexMap_t {
	// texture diffuse
	sampler2D texture_diffuse1;
	sampler2D texture_diffuse2;
	// texture specular
	sampler2D texture_specular1;
	sampler2D texture_specular2;
	// texture normal
	sampler2D texture_normal1;
	sampler2D texture_normal2;
	// texture height
	sampler2D texture_height1;
	sampler2D texture_height2;
	// texture emission
	sampler2D texture_emission1;
	sampler2D texture_emission2;
	// To be added ...
};

/** Uniform variables */

// Camera
uniform vec3 uCameraPos;

// Lighting
uniform Directional_Light_t uDirectionalLight;
uniform Spot_Light_t uSpotLight;
uniform Point_Light_t uPointLight;
uniform Point_Light_t uPointLights[4];
uniform bool uTorch;
uniform bool uBlinn;
uniform float uGamma;

// Shadow
uniform sampler2D uShadowMap;

float CalcShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 lightDir, vec3 normal);

// Texture (Model Importer specified)
uniform MatTexMap_t uMaterial;

/** Stream variables */

out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
	vec4 FragPosLightSpace;
} fs_in;

void main() {

	vec3 normal = normalize(fs_in.Normal);
	vec3 viewDir = normalize(uCameraPos - fs_in.FragPos);
	vec3 resultColor = vec3(0.0, 0.0, 0.0);

	// Directional lighting
	vec3 directionalLightColor = CalcDirectionalLight(uDirectionalLight, normal, viewDir,
		uMaterial.texture_diffuse1, uMaterial.texture_specular1, uMaterial.texture_emission1);

	// Spot lighting
	vec3 spotLightColor = vec3(0.0, 0.0, 0.0);
	if (uTorch)
		spotLightColor = CalcSpotLight(uSpotLight, normal, viewDir,
			uMaterial.texture_diffuse1, uMaterial.texture_specular1);

	// Point lighting
	vec3 pointLightColor = vec3(0.0, 0.0, 0.0);
	pointLightColor = CalcPointLight(uPointLight, normal, viewDir,
		uMaterial.texture_diffuse1, uMaterial.texture_specular1,
		fs_in.FragPosLightSpace, uShadowMap);

	resultColor = spotLightColor + pointLightColor;

	// Gamma correction
	resultColor.xyz = pow(resultColor.xyz, vec3(1.0 / uGamma));

	// Result
	FragColor = vec4(resultColor, 1.0);
}

float CalcShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 lightDir, vec3 normal) {
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	//
	if (projCoords.z > 1.0) return 0.0;
	// transform to [0, 1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective
	//float closestDepth = texture(shadowMap, projCoords.xy).r;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	//float shadow = shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	return shadow;
}

vec3 CalcDirectionalLight(Directional_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular, sampler2D emission) {

	vec3 lightDir = normalize(-light.direction);
	// ambient
	vec3 ambientColor = light.ambient * vec3(texture(diffuse, fs_in.TexCoords));
	// diffuse
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec3 diffuseColor = diffEff * light.diffuse * vec3(texture(diffuse, fs_in.TexCoords));
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
	vec3 specularColor = specEff * light.specular * vec3(texture(specular, fs_in.TexCoords));
	// emission
	vec3 emissionColor = vec3(0.0);
	if (texture(specular, fs_in.TexCoords).r == 0.0)
		emissionColor = texture(emission, fs_in.TexCoords).rgb;
	// result
	return ambientColor + diffuseColor + specularColor + emissionColor;
}

vec3 CalcPointLight(Point_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular,
	vec4 fragPosLightSpace, sampler2D shadowMap) {

	vec3 lightDir = normalize(light.position - fs_in.FragPos);
	// Physics
	float distance = length(light.position - fs_in.FragPos);
	//float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*distance*distance);
	float attenuation = 1.0 / distance * distance;
	// ambient
	vec3 ambientColor = light.ambient * vec3(texture(diffuse, fs_in.TexCoords));
	// diffuse
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec3 diffuseColor = diffEff * light.diffuse * vec3(texture(diffuse, fs_in.TexCoords));
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	//float discardFactor = dot(reflectDir, normal); // if (discardFactor < 0.0) spefEff = 0.0;
	float specEff = 0.0;
	if (uBlinn)
		specEff = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	else
		specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
	vec3 specularColor = specEff * light.specular * vec3(texture(specular, fs_in.TexCoords));
	// shadow
	float shadow = CalcShadow(fragPosLightSpace, shadowMap, lightDir, normal);
	// result
	return  ambientColor + (diffuseColor + specularColor) * attenuation * (1.0 - shadow);
}

vec3 CalcSpotLight(Spot_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(light.position - fs_in.FragPos);
	// Physics
	float distance = length(light.position - fs_in.FragPos);
	float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*distance*distance);
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.innerCutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	// Ambient lighting
	vec3 ambientColor = light.ambient * vec3(texture(diffuse, fs_in.TexCoords));
	// Diffuse lighting
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec3 diffuseColor = diffEff * light.diffuse * vec3(texture(diffuse, fs_in.TexCoords));
	// Specular lighting
	vec3 reflectDir = reflect(-lightDir, normal);
	float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
	vec3 specularColor = specEff * light.specular * vec3(texture(specular, fs_in.TexCoords));
	// Result lighting
	return attenuation * (ambientColor + (diffuseColor + specularColor) * intensity);
}
