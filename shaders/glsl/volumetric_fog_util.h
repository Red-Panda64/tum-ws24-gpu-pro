#define VOLUME_WIDTH  512.0
#define VOLUME_HEIGHT 256.0
#define VOLUME_DEPTH  128.0

#define FOG_RANGE 1000.0 // TODO: Make it parameterizable? If we add ImGui
#define DEPTH_PACK_EXPONENT 2.0

#define PI 3.14159265358979323846

vec3 ndcFromThreadID(uvec3 threadID)
{
    // Map from thread id to the Vulkan NDC range X: [-1,1], Y: [-1,1], Z: [0,1]
    return (vec3(threadID.xyz)) * vec3(2.0f / VOLUME_WIDTH, 2.0f / VOLUME_HEIGHT, 1.0f / VOLUME_DEPTH) + vec3(-1, -1, 0); 
}

vec3 ndcFromThreadID(vec3 threadID)
{
    // Similar to above but used when jittering is used for sampling.
    return (threadID.xyz + 0.5f) * vec3(2.0f / VOLUME_WIDTH, 2.0f / VOLUME_HEIGHT, 1.0f / VOLUME_DEPTH) + vec3(-1, -1, 0);
}

vec3 worldPositionFromNdcCoords(vec2 ndcPos, float linearDepth)
{
    vec3 eyeRay = (cameraXAxis.xyz * ndcPos.xxx +
        cameraYAxis.xyz * ndcPos.yyy +
        cameraZAxis.xyz);

    vec3 viewRay = eyeRay * linearDepth;
    vec3 worldPos = cameraPos.xyz + viewRay;

    return worldPos;
}

float volumeZPosToDepth(float volumePosZ)
{
    return pow(abs(volumePosZ), DEPTH_PACK_EXPONENT) * FOG_RANGE;
}

float depthToVolumeZPos(float depth)
{
    return pow(abs(depth / FOG_RANGE), 1.0f / DEPTH_PACK_EXPONENT);
}

vec3 volumeTextureSpaceFromScreenSpace(vec3 volumeSS)
{
    return clamp(volumeSS, 0.0, 1.0) * vec3((VOLUME_WIDTH - 1) / VOLUME_WIDTH, (VOLUME_HEIGHT - 1) / VOLUME_HEIGHT, (VOLUME_DEPTH - 1) / VOLUME_DEPTH) + vec3(0.5f / VOLUME_WIDTH, 0.5f / VOLUME_HEIGHT, 0.5f / VOLUME_DEPTH);
}

float getPhaseFunction(float cosPhi, float gFactor)
{
    float gFactor2 = gFactor * gFactor;
    return (1 - gFactor2) / pow(abs(1 + gFactor2 - 2 * gFactor * cosPhi), 1.5f) * (1.0f / 4.0f * PI);
}

vec4 getRotatedPhaseFunctionSH(vec3 dir, float g)
{
    // Returns properly rotated spherical harmonics (from Henyey-Greenstein Zonal SH expansion) 
    // for given view vector direction.
    return vec4(1.0f, dir.y, dir.z, dir.x) * vec4(1.0f, g, g, g);
}

vec3 applyFog(sampler3D scatteringVolume, vec3 color, vec3 screenSpace) {
	vec3 uvw = volumeTextureSpaceFromScreenSpace(screenSpace);
	vec4 inScatteringTransmittance = texture(scatteringVolume, uvw);
	return vec3(color * inScatteringTransmittance.a + inScatteringTransmittance.rgb);
}
