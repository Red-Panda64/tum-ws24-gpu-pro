#version 460
#extension GL_EXT_nonuniform_qualifier: enable


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

// Each mesh has 2 textures (diffuse and specular). Each mesh, can fetch its textures with index: 2 * gl_drawID
layout(set = 1, binding = 0) uniform sampler2D textures[];

// Per-Instance Transform Matrices. Index with gl_InstanceID
layout(set = 2, binding = 0) readonly buffer ModelTransforms
{
    mat4 model[];
} modelTransforms;


layout(location = 0) out VOut
{
    vec3 normal;
    vec3 fragWorldPos; // for light calculations
    vec2 uv;
    flat int drawID;
} vOut;

void main()
{
    vec4 intermediateWorldPos = modelTransforms.model[gl_InstanceIndex] * vec4(vertex_position, 1.0);
    vOut.normal = mat3(inverse(transpose(modelTransforms.model[gl_InstanceIndex]))) * vertex_normal;
    vOut.fragWorldPos = vec3(intermediateWorldPos);
    vOut.uv = vertex_uv;
    vOut.drawID = gl_DrawID;
    gl_Position = scene.projectionView * intermediateWorldPos;
}
