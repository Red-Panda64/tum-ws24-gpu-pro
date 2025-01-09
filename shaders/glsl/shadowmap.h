
layout(set = 3, binding = 0) uniform DirShadower
{
    mat4 projectionView;
} shadower;

//layout(set = 3, binding = 1) uniform sampler2Dshadow shadowMap;
layout(set = 3, binding = 1) uniform sampler2D shadowMap;

float getShadowValue(vec4 worldPosition, float bias) {
    // TODO: slope dependent bias
    vec4 lightspacePositionH = shadower.projectionView * worldPosition;
    vec3 lightspacePosition = lightspacePositionH.xyz / lightspacePositionH.w;
    vec2 shadowUVs = lightspacePosition.xy * 0.5f + vec2(0.5f);
    //return texture(shadowMap, vec3(shadowUVs, lightspacePosition.z - bias));
    return (texture(shadowMap, vec2(shadowUVs)).r > lightspacePosition.z - bias) ? 1.0 : 0.0;
}
