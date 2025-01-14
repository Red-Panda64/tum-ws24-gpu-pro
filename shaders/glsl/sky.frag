#version 460
#include "shadow_map.h"
#include "scene.h"

layout(set = 0, binding = 0) uniform UScene
{
    Scene scene;
};

layout(set = 0, binding = 1) uniform sampler3D scatteringVolume;

layout(set = 0, binding = 2) uniform VolumeGenerationInputs
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

layout(location = 0) in VIn
{
    vec3 fragWorldPos;
} vIn;

layout(location = 0) out vec4 color;

void main()
{
	vec3 viewDirUN = vIn.fragWorldPos - scene.camPos;
	float normalizedY = viewDirUN.y / length(viewDirUN);
    float mixAlpha = smoothstep(0.0, 1.0, normalizedY * 0.5f + 0.25f);
    mixAlpha = 1.0f - pow(1.0f - mixAlpha, 100.0f);
	vec3 upColor = vec3(0.2f, 0.35f, 0.75f);
	vec3 downColor = vec3(0.65f, 0.65f, 0.65f);
	color = vec4(mix(downColor, upColor, vec3(mixAlpha, mixAlpha, mixAlpha)), 1.0);
}
