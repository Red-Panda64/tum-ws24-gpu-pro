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

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicMap;
layout(set = 1, binding = 3) uniform sampler2D roughnessMap;
layout(set = 1, binding = 4) uniform sampler2D aoMap;

layout(location = 0) in VIn
{
    vec3 normal;
    vec3 fragWorldPos; // for light calculations
	vec2 uv;
} vIn;

layout(location = 0) out vec4 color;

#define PI 3.14159265358979323846

/*
	Easy trick to get tangent-normals to world-space. Not the most performant.
	TODO: Perform a proper normal mapping
*/
vec3 fetchNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, vIn.uv).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(vIn.fragWorldPos);
    vec3 Q2  = dFdy(vIn.fragWorldPos);
    vec2 st1 = dFdx(vIn.uv);
    vec2 st2 = dFdy(vIn.uv);

    vec3 N   = normalize(vIn.normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

/*
	Trowbridge-Reitz GGX normal distribution function
	
	Note that, compared to the original formula we are squaring roughness as the lighting looks more correct based on observations of Disney.
*/
float distributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return num / denom;
}

/*
	Schlick-GGX Geometry Function with respect to some direction V (directly using NdotV here as we only need its dot with normal of the surface). This will be used as a helper in Smith's method.
*/
float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
}

/*
	Smith's Method that takes both view direction (geometry obstruction) and light direction (geometry shadowing).
*/
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	// Geometry Shadowing
	float ggx1 = geometrySchlickGGX(NdotL, roughness);
	// Geometry Obstruction
	float ggx2 = geometrySchlickGGX(NdotV, roughness);

	return ggx1 * ggx2;
}

/*
	Fresnel-Schlick approximation of Fresnel Equation. 
*/
vec3 fresnelSchlick(float cosThetaHV, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosThetaHV, 0.0, 1.0), 5.0);
}

vec3 computeCookTorranceReflectance(vec3 N, vec3 V, vec3 L, vec3 H, vec3 radiance, vec3 albedo, vec3 F0, float roughness, float metallic)
{
	vec3 Lo = vec3(0.0f);

	// Cook-Torrance BRDF
	float NDF = distributionGGX(N, H, roughness); // normal distribution function
	float G = geometrySmith(N, V, L, roughness); // geometry function
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
	vec3 specular = numerator / denominator;

	// kS is equal to the Fresnel
	vec3 kS = F;
	// As PBR model, energy is conserved. So, kS + kD = 1.0
	vec3 kD = vec3(1.0f) - kS;
	// As we model both non-metal and metal materials in the same model, we need to take into account that metal materials has no diffuse light. 
	kD *= (1.0f - metallic); // linearly blend between 0 and kD value. If pure-metal, we will have no diffuse light.

	// cosine factor in the reflectance integral
	float NdotL = max(dot(N, L), 0.0f);

	// Outgoing irradiance
	Lo = (kD * albedo / PI + specular) * radiance * NdotL; // Note that BRDF already contains kS from the Fresnel term, so we won't multiply by kS again.

	return Lo;
}

vec3 computeIrradiance(vec3 N, vec3 V, vec3 albedo, vec3 F0, float roughness, float metallic)
{
	vec3 Lo = vec3(0.0f);
	// Directional Light Irradiance
	vec3 L = normalize(-scene.dirLight.direction);
	vec3 H = normalize(V + L);
	// Directional lights has a constant radiance which is their radiant flux (light color). 
	vec3 radiance = scene.dirLight.color;
	Lo += computeCookTorranceReflectance(N, V, L, H, radiance, albedo, F0, roughness, metallic);

	// Point-Light Irradiance
	for(int i = 0; i < scene.nrPointLights; ++i)
	{
		PointLight P = scene.pointLights[i];
		// Per-light radiance
		vec3 L = (P.position - vIn.fragWorldPos);
		float dist = length(L);
		L = normalize(L);
		vec3 H = normalize(V + L);
		// Using quadratic attenuation for more control over the lighting distance
		float attenuation = 1.0f / (P.attenuationFactors[0] + P.attenuationFactors[1] * dist + P.attenuationFactors[2] * dist * dist);
		// Point Lights have the same radiance regardles of the angle we look at it. This is because their radiant intensity and radiant flux (we model radiant flux as the light color basically) are the same and constant.
		// So, we can use radiant flux (light color) as the light intensity in the radiance formula. Note that this assumption holds as we assume point lights has no area or volume.
		vec3 radiance = P.color * attenuation;
		// Accumulate into outgoing irradiance
		Lo += computeCookTorranceReflectance(N, V, L, H, radiance, albedo, F0, roughness, metallic);
	}

	return Lo;
}

void main()
{
	//Fundamental variables
	vec3 albedo = pow(texture(albedoMap, vIn.uv).rgb, vec3(2.2f)); // Map albedo from sRGB to linear as we will do our light computation in linear space
	float metallic = texture(metallicMap, vIn.uv).r;
	float roughness = texture(roughnessMap, vIn.uv).r;
	float ao = texture(aoMap, vIn.uv).r;

	vec3 N = fetchNormalFromMap();
	vec3 V = normalize(scene.camPos - vIn.fragWorldPos);

	// Base reflectance. If the material is non-metallic, we use F0 of 0.04 and if it is metal we use the albedo color as F0. As albedo maps may contain both non-metallic and metallic parts, we use linear 
	// interpolation between non-metallic F0 and metallic F0 albedo color. (metallic workflow)
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = computeIrradiance(N, V, albedo, F0, roughness, metallic);
	vec3 ambient = vec3(scene.ambientFactor) * albedo * ao;

	vec3 outColor = ambient + Lo;

	// Add Fog (Need to test how to incorporate HDR tonemapping and gamma correction. I think first operate on linear space and finally apply HDR tonemapping and gamma correction alltogether)

	float linearDepth = dot(vIn.fragWorldPos - scene.camPos, cameraZAxis);
	outColor = applyFog(scatteringVolume, outColor, vec3(gl_FragCoord.xy / scene.viewport, clamp(depthToVolumeZPos(linearDepth), 0.0, 1.0)));

	// HDR tonemapping (Reinhardt operator)
	outColor = outColor / (outColor + vec3(1.0f));
	// Gamma correction (map from linear to sRGB)
	outColor = pow(outColor, vec3(1.0f / 2.2f));

	color = vec4(outColor, 1.0f);
}
