#version 330 core

/** Directional Light */

struct Directional_Light_t {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

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

/** Texture mapping */

struct MatTexMap_t {
	// texture diffuse
	sampler2D texture_diffuse1;
	sampler2D texture_diffuse2;
	sampler2D texture_diffuse3;
	sampler2D texture_diffuse4;
	// texture specular
	sampler2D texture_specular1;
	sampler2D texture_specular2;
	sampler2D texture_specular3;
	sampler2D texture_specular4;
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

/** Lighting functions */

vec3 CalcDirectionalLight(
	Directional_Light_t light,
	vec3 normal, vec3 viewDir,
	vec2 texCoords,
	sampler2D diffuse, sampler2D specular);

vec3 CalcPointLight(
	Point_Light_t light,
	vec3 normal, vec3 viewDir, vec3 fragPos,
	vec2 texCoords,
	sampler2D diffuse, sampler2D specular);

vec3 CalcSpotLight(
	Spot_Light_t light,
	vec3 normal, vec3 viewDir, vec3 fragPos,
	vec2 texCoords,
	sampler2D diffuse, sampler2D specular);

vec3 CalcEmission(
	vec2 texCoords,
	sampler2D emission);

/** Uniform variables */

// Camera
uniform vec3 uCameraPos;

// Lighting
uniform Directional_Light_t uDirectionalLight;
uniform Spot_Light_t uSpotLight;
uniform Point_Light_t uPointLight;
uniform Point_Light_t uPointLights[4];
uniform bool uEnableTorch;
uniform bool uEnableBlinn;
uniform bool uEnableNormal;
uniform float uGamma;

// Texture (Model Importer specified)
uniform MatTexMap_t uMaterial;

/** Stream variables */

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

void main() {

	vec3 normal = normalize(fs_in.Normal);
	vec3 viewDir = normalize(uCameraPos - fs_in.FragPos);
	vec3 resultColor = vec3(0.0, 0.0, 0.0);

	if (uEnableNormal) {
		normal = texture(uMaterial.texture_normal1, fs_in.TexCoords).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		normal = normalize(fs_in.TBN * normal);
	}

	// Directional lighting
	resultColor += CalcDirectionalLight(uDirectionalLight, normal, viewDir, fs_in.TexCoords,
		uMaterial.texture_diffuse1, uMaterial.texture_specular1);

	// Spot lighting
	if (uEnableTorch)
		resultColor += CalcSpotLight(uSpotLight, normal, viewDir, fs_in.FragPos, fs_in.TexCoords,
			uMaterial.texture_diffuse1, uMaterial.texture_specular1);

	// Point lighting
	for (int i=0; i<4; i++) {
		resultColor += CalcPointLight(uPointLights[i], normal, viewDir, fs_in.FragPos, fs_in.TexCoords,
			uMaterial.texture_diffuse1, uMaterial.texture_specular1);
	}

	// Gamma correction
	resultColor.xyz = pow(resultColor.xyz, vec3(1.0 / uGamma));

	// Result
	FragColor = vec4(resultColor, 1.0);
}

vec3 CalcDirectionalLight(Directional_Light_t light, vec3 normal, vec3 viewDir,
	vec2 texCoords, sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(-light.direction);
	// ambient
	vec3 ambientColor = light.ambient * vec3(texture(diffuse, texCoords));
	// diffuse
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec3 diffuseColor = diffEff * light.diffuse * vec3(texture(diffuse, texCoords));
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
	vec3 specularColor = specEff * light.specular * vec3(texture(specular, texCoords));
	// result
	return ambientColor + diffuseColor + specularColor;
}

vec3 CalcPointLight(Point_Light_t light, vec3 normal, vec3 viewDir, vec3 fragPos,
	vec2 texCoords, sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(light.position - fragPos);
	// Physics
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*distance*distance);
	//float attenuation = 1.0 / (distance * distance);
	// ambient
	vec3 ambientColor = light.ambient * vec3(texture(diffuse, texCoords));
	// diffuse
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec3 diffuseColor = diffEff * light.diffuse * vec3(texture(diffuse, texCoords));
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	//float discardFactor = dot(reflectDir, normal); // if (discardFactor < 0.0) spefEff = 0.0;
	float specEff = 0.0;
	if (uEnableBlinn)
		specEff = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	else
		specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
	vec3 specularColor = specEff * light.specular * vec3(texture(specular, texCoords));
	// result
	return  ambientColor + (diffuseColor + specularColor) * attenuation;
}

vec3 CalcSpotLight(Spot_Light_t light, vec3 normal, vec3 viewDir, vec3 fragPos,
	vec2 texCoords, sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(light.position - fragPos);
	// Physics
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*distance*distance);
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.innerCutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	// Ambient lighting
	vec3 ambientColor = light.ambient * vec3(texture(diffuse, texCoords));
	// Diffuse lighting
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec3 diffuseColor = diffEff * light.diffuse * vec3(texture(diffuse, texCoords));
	// Specular lighting
	vec3 reflectDir = reflect(-lightDir, normal);
	float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
	vec3 specularColor = specEff * light.specular * vec3(texture(specular, texCoords));
	// Result lighting
	return attenuation * (ambientColor + (diffuseColor + specularColor) * intensity);
}

vec3 CalcEmission(vec2 texCoords, sampler2D emission) {
	// Customize here
	vec3 emissionColor = texture(emission, texCoords).rgb;
	return emissionColor;
}
