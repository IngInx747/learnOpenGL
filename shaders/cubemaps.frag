#version 330 core

/** Methods */

float LinearizeDepth(float depth);

/** Directional Light */

struct Directional_Light_t {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

vec4 CalcDirectionalLight(Directional_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular);

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

vec4 CalcPointLight(Point_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular);

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

vec4 CalcSpotLight(Spot_Light_t light, vec3 normal, vec3 viewDir,
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
	// To be added ...
};

/** Uniform variables */

// Camera
uniform vec3 uCameraPos;

// Lighting
#define NR_POINT_LIGHTS 4
uniform Directional_Light_t uDirectionalLight;
uniform Spot_Light_t uSpotLight;
uniform Point_Light_t uPointLights[NR_POINT_LIGHTS];

// Texture (Model Importer specified)
uniform MatTexMap_t uMaterial;

/** Stream variables */

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

void main() {

	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(uCameraPos - FragPos);
	vec4 resultColor = vec4(0.0);

	// Directional lighting
	vec4 directionalLightColor = CalcDirectionalLight(uDirectionalLight, normal, viewDir,
		uMaterial.texture_diffuse1, uMaterial.texture_specular1);

	// Spot lighting
	vec4 spotLightColor = CalcSpotLight(uSpotLight, normal, viewDir,
		uMaterial.texture_diffuse1, uMaterial.texture_specular1);

	// Point lighting
	/**
	for (int i=0; i<NR_POINT_LIGHTS; i++) {
		resultColor += CalcPointLight(uPointLights[i], normal, viewDir,
			uMaterial.texture_diffuse1, uMaterial.texture_specular1);
	}*/

	// Depth testing
	//vec4 fadingEffect = vec4(LinearizeDepth(gl_FragCoord.z), 1.0);
	//vec4 fadingEffect = vec4(vec3(1.0 - gl_FragCoord.z), 1.0);

	// Result
	resultColor = directionalLightColor + spotLightColor;

	// Transparency process
	//if (resultColor.a < 0.01) discard;
	FragColor = resultColor;
}

float LinearizeDepth(float depth) {

	float near = 0.1;
	float far  = 100.0;

	float z = depth * 2.0 - 1.0; // back to NDC
	return (2.0 * near) / (far + near - z * (far - near));
}

vec4 CalcDirectionalLight(Directional_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(-light.direction);
	// ambient
	vec4 ambientColor = vec4(light.ambient, 1.0) * texture(diffuse, TexCoords);
	// diffuse
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec4 diffuseColor = diffEff * vec4(light.diffuse, 1.0) * texture(diffuse, TexCoords);
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float discardFactor = dot(reflectDir, normal);
	vec4 specularColor;
	if (discardFactor < 0.0)
		specularColor = vec4(0.0, 0.0, 0.0, 0.0);
	else {
		float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
		specularColor = specEff * vec4(light.specular, 1.0) * texture(specular, TexCoords);
	}
	// result
	return ambientColor + diffuseColor + specularColor;
}

vec4 CalcPointLight(Point_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(light.position - FragPos);
	// Physics
	float distance = length(light.position - FragPos);
	float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*distance*distance);
	// ambient
	vec4 ambientColor = vec4(light.ambient, 1.0) * texture(diffuse, TexCoords);
	// diffuse
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec4 diffuseColor = diffEff * vec4(light.diffuse, 1.0) * texture(diffuse, TexCoords);
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float discardFactor = dot(reflectDir, normal);
	vec4 specularColor;
	if (discardFactor < 0.0)
		specularColor = vec4(0.0, 0.0, 0.0, 0.0);
	else {
		float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
		specularColor = specEff * vec4(light.specular, 1.0) * texture(specular, TexCoords);
	}
	// result
	return attenuation * (ambientColor + diffuseColor + specularColor);
}

vec4 CalcSpotLight(Spot_Light_t light, vec3 normal, vec3 viewDir,
	sampler2D diffuse, sampler2D specular) {

	vec3 lightDir = normalize(light.position - FragPos);
	// Physics
	float distance = length(light.position - FragPos);
	float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*distance*distance);
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.innerCutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	// Ambient lighting
	vec4 ambientColor = vec4(light.ambient, 1.0) * texture(diffuse, TexCoords);
	// Diffuse lighting
	float diffEff = max(dot(normal, lightDir), 0.0);
	vec4 diffuseColor = diffEff * vec4(light.diffuse, 1.0) * texture(diffuse, TexCoords);
	// Specular lighting
	vec3 reflectDir = reflect(-lightDir, normal);
	float discardFactor = dot(reflectDir, normal);
	vec4 specularColor;
	if (discardFactor < 0.0)
		specularColor = vec4(0.0, 0.0, 0.0, 0.0);
	else {
		float specEff = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
		specularColor = specEff * vec4(light.specular, 1.0) * texture(specular, TexCoords);
	}
	// Result lighting
	return attenuation * (ambientColor + (diffuseColor + specularColor) * intensity);
}
