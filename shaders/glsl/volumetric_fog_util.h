/*
    The MIT License (MIT)

    Copyright (c) 2014 bartwronski

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#define FOG_RANGE 300.0 // TODO: Make it parameterizable? If we add ImGui
#define DEPTH_PACK_EXPONENT 1.8
#define VOLUME_WIDTH  512.0
#define VOLUME_HEIGHT 256.0
#define VOLUME_DEPTH  128.0

#define PI 3.14159265358979323846

vec3 ndcFromThreadID(uvec3 threadID, uvec3 resolution)
{
    // Map from thread id to the Vulkan NDC range X: [-1,1], Y: [-1,1], Z: [0,1]
    return (vec3(threadID.xyz)) * (vec3(2.0f, 2.0f, 1.0f) / vec3(resolution)) + vec3(-1, -1, 0);
}

vec3 ndcFromThreadID(vec3 threadID, uvec3 resolution)
{
    // Similar to above but used when jittering is used for sampling.
    return (threadID.xyz + 0.5f) * (vec3(2.0f, 2.0f, 1.0f) / vec3(resolution)) + vec3(-1, -1, 0);
}

vec3 worldPositionFromNdcCoords(vec2 ndcPos, float linearDepth)
{
    vec3 eyeRay = (cameraXAxis.xyz * ndcPos.xxx +
        cameraYAxis.xyz * ndcPos.yyy +
        cameraZAxis.xyz);

    vec3 viewRay = eyeRay * (zNear + linearDepth);
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
	vec3 uvw = screenSpace;//volumeTextureSpaceFromScreenSpace(screenSpace);
	vec4 inScatteringTransmittance = texture(scatteringVolume, uvw);
	return vec3(color * inScatteringTransmittance.a + inScatteringTransmittance.rgb);
}
