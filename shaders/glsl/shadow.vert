#version 460

layout(location = 0) in vec3 vertex_position;

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projectionView;
} scene;

// TODO: compute mvp on host
layout(set = 1, binding = 0) uniform ModelTransforms
{
    mat4 model;
} modelTransform;

void main()
{
    vec4 intermediateWorldPos = modelTransform.model * vec4(vertex_position, 1.0);
    gl_Position = scene.projectionView * intermediateWorldPos;
}
