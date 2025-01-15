#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_uv;
layout(location = 2) in vec3 vertex_normal;
layout(location = 3) in vec3 vertex_tangent;

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


layout(location = 0) out VOut
{
    vec3 normal;
    vec3 tangent;
    vec3 fragWorldPos; // for light calculations
    vec2 uv;
} vOut;

void main()
{
    vec4 intermediateWorldPos = modelTransform.model * vec4(vertex_position, 1.0);
    mat3 normalTransformation = mat3(inverse(transpose(modelTransform.model))); 
    vOut.normal = normalTransformation * vertex_normal;
    vOut.tangent = normalTransformation * vertex_tangent;
    vOut.fragWorldPos = vec3(intermediateWorldPos);
    vOut.uv = vertex_uv;
    gl_Position = scene.projectionView * intermediateWorldPos;
}
