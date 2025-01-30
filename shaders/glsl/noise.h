
/*
FX implementation of Ken Perlin's "Improved Noise"
sgg 6/26/04
http://mrl.nyu.edu/~perlin/noise/
*/

uniform sampler2D permSamplerTex;

static const uint permutation[] = { 151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
    190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
    77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
    102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
    135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
    5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
    223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
    251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
    49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

// gradients for 3d noise
static const vec3 g[] = {
    vec3(1, 1, 0),
    vec3(-1, 1, 0),
    vec3(1, -1, 0),
    vec3(-1, -1, 0),
    vec3(1, 0, 1),
    vec3(-1, 0, 1),
    vec3(1, 0, -1),
    vec3(-1, 0, -1),
    vec3(0, 1, 1),
    vec3(0, -1, 1),
    vec3(0, 1, -1),
    vec3(0, -1, -1),
    vec3(1, 1, 0),
    vec3(0, -1, 1),
    vec3(-1, 1, 0),
    vec3(0, -1, -1),
};

vec4 gradperm(float u, vec3 p)
{
    uint position = uint(u * 255.0f) % 256;
    vec4 gradPermSample = vec4(g[permutation[position] % 16], 0);
    return dot(gradPermSample, p);
}

vec3 fade(vec3 t)
{
    return t * t * t * (t * (t * 6 - 15) + 10); // new curve
}

float perm(float x)
{
    return permSamplerTex.SampleLevel(pointWrapSampler, vec2(x,0.5f), 0);
}

//float gradperm(float x, vec3 p)
//{
//    return dot(permGradSamplerTex.SampleLevel(pointWrapSampler, vec2(x, 0.5f), 0).xyz, p);
//}

// optimized version
float inoise(vec3 p)
{
    vec3 P = fmod(floor(p), 256.0);	// FIND UNIT CUBE THAT CONTAINS POINT
    p -= floor(p);                      // FIND RELATIVE X,Y,Z OF POINT IN CUBE.
    vec3 f = fade(p);                 // COMPUTE FADE CURVES FOR EACH OF X,Y,Z.

    P = P / 256.0;
    const float one = 1.0 / 256.0;

    // HASH COORDINATES OF THE 8 CUBE CORNERS
    vec4 AA = perm2d(P.xy) + P.z;

    // AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
    return lerp(lerp(lerp(gradperm(AA.x, p),
        gradperm(AA.z, p + vec3(-1, 0, 0)), f.x),
        lerp(gradperm(AA.y, p + vec3(0, -1, 0)),
        gradperm(AA.w, p + vec3(-1, -1, 0)), f.x), f.y),

        lerp(lerp(gradperm(AA.x + one, p + vec3(0, 0, -1)),
        gradperm(AA.z + one, p + vec3(-1, 0, -1)), f.x),
        lerp(gradperm(AA.y + one, p + vec3(0, -1, -1)),
        gradperm(AA.w + one, p + vec3(-1, -1, -1)), f.x), f.y), f.z);
}

float turbulence(vec3 p, int octaves, float lacunarity = 2.0, float gain = 0.5)
{
    float sum = 0;
    float freq = 1.0, amp = 1.0;
    for (int i = 0; i<octaves; i++) {
        sum += abs(inoise(p*freq))*amp;
        freq *= lacunarity;
        amp *= gain;
    }
    return sum;
}
