#version 460
#extension GL_GOOGLE_include_directive : enable
#include "scene.h"

vec3 vertices[] = {
    vec3(-1.0f, -1.0f, 1.0f),
    vec3( 1.0f, -1.0f, 1.0f),
    vec3(-1.0f,  1.0f, 1.0f),
    vec3( 1.0f,  1.0f, 1.0f),
};

int indices[] = { 0, 2, 1, 1, 2, 3 };

layout(set = 0, binding = 0) uniform UScene
{
    Scene scene;
};

layout(location = 0) out VOut
{
    vec3 fragWorldPos; // for light calculations
} vOut;

void main()
{
    vec3 pos = vertices[indices[gl_VertexIndex]];
    vec4 fragWorldPos = scene.invProjectionView * vec4(pos, 1.0f);
    vOut.fragWorldPos = fragWorldPos.xyz / fragWorldPos.w;
    gl_Position = vec4(pos, 1.0f);
}
