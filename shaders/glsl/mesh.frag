#version 460
#include "shadow_map.h"
#include "scene.h"

layout(set = 0, binding = 0) uniform UScene
{
    Scene scene;
};

layout(set = 0, binding = 1) uniform DirShadower
{
    mat4 lightPV;
} shadower;

layout(set = 0, binding = 2) uniform sampler2D shadowMap;

layout(set = 0, binding = 3) uniform sampler3D scatteringVolume;

layout(set = 0, binding = 4) uniform VolumeGenerationInputs
{
    uvec3 resolution;
    vec3 cameraPos;
    vec3 cameraXAxis;
    vec3 cameraYAxis;
    vec3 cameraZAxis;
    DirLight dirLight;
    float time;
    int frameNumber;
};

#include "volumetric_fog_util.h"

layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D specularTexture;

layout(location = 0) in VIn
{
    vec3 normal;
    vec3 fragWorldPos; // for light calculations
	vec2 uv;
} vIn;

layout(location = 0) out vec4 color;

vec3 calculateDirLight(DirLight light, vec3 normal, vec3 fragToCamera, vec3 diffuseTextureValue, vec3 specularTextureValue)
{
	vec3 fragToLight = normalize(-light.direction);
	//Diffuse shading
	float diffCoeff = max(dot(fragToLight, normal), 0.0f);
	//Specular shading
	vec3 reflected = reflect(-fragToLight, normal);
	vec3 halfwayDir = normalize(fragToLight + fragToCamera);
	float specularCoeff = pow(max(dot(halfwayDir, normal), 0.0f), 32);
	//Combine results
	vec3 ambient = light.color * scene.ambientFactor * diffuseTextureValue;
	vec3 diffuse = light.color * diffCoeff * diffuseTextureValue;
	vec3 specular = light.color * specularCoeff * specularTextureValue;

	float shadowValue = getShadowValue(shadower.lightPV, shadowMap, vec4(vIn.fragWorldPos, 1.0f), 0.001f);
	return (ambient + shadowValue * (diffuse + specular));
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 fragToCamera, vec3 diffuseTextureValue, vec3 specularTextureValue)
{
	vec3 fragToLight = normalize(light.position - fragPosition);
	//Diffuse shading
	float diffCoeff = max(dot(fragToLight, normal), 0.0f);
	//Specular shading
	vec3 reflected = reflect(-fragToLight, normal);
	vec3 halfwayDir = normalize(fragToLight + fragToCamera);
	float specularCoeff = pow(max(dot(halfwayDir, normal), 0.0f), 32);

	//Attenuation
	float dist = length(light.position - fragPosition);
	float attenuation = 1.0f / (light.attenuationFactors[0] + light.attenuationFactors[1] * dist + light.attenuationFactors[2] * dist * dist);

	//Combine
	vec3 ambient = light.color * scene.ambientFactor * diffuseTextureValue;
	vec3 diffuse = light.color * diffCoeff * diffuseTextureValue;
	vec3 specular = light.color * specularCoeff * specularTextureValue;

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	
	return (ambient + diffuse + specular);

}

void main()
{
	//Fundamental variables
	vec3 n = normalize(vIn.normal);
	vec3 fragToCamera = normalize(scene.camPos - vIn.fragWorldPos);

	vec3 diffuseTextureValue = vec3(texture(diffuseTexture, vIn.uv));
	vec3 specularTextureValue = vec3(texture(specularTexture, vIn.uv));

	vec3 result = calculateDirLight(scene.dirLight, n, fragToCamera, diffuseTextureValue, specularTextureValue);

	for(int i = 0; i < scene.nrPointLights; ++i)
	{
		result += calculatePointLight(scene.pointLights[i], n, vIn.fragWorldPos, fragToCamera, diffuseTextureValue, specularTextureValue);
	}

    float linearDepth = dot(scene.camPos - vIn.fragWorldPos, cameraZAxis);
	result = applyFog(scatteringVolume, result, vec3(gl_FragCoord.xy / scene.viewport, clamp(depthToVolumeZPos(linearDepth), 0.0, 1.0)));
	color = vec4(result, 1.0f);
}
