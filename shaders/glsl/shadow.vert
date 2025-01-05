#version 460

layout(location = 0) in vec3 vertex_position;

struct DirLight
{
    vec3 direction;
    vec3 color;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    vec3 attenuationFactors; // constant, linear and quadratic in order
};

#define MAX_NR_OF_POINT_LIGHTS 16
layout(set = 0, binding = 0) uniform Scene
{
    mat4 projectionView;
    vec3 camPos;
    DirLight dirLight;
    PointLight pointLights[MAX_NR_OF_POINT_LIGHTS];
    int nrPointLights;
    float ambientFactor;
} scene;

layout(set = 2, binding = 0) uniform ModelTransforms
{
    mat4 model;
} modelTransform;

void main()
{
    vec4 intermediateWorldPos = modelTransform.model * vec4(vertex_position, 1.0);
    gl_Position = scene.projectionView * intermediateWorldPos;
}
