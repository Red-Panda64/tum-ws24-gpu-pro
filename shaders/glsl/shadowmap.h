
float getShadowValue(mat4 lightPV, sampler2D shadowMap, vec4 worldPosition, float bias) {
    // TODO: slope dependent bias
    vec4 lightspacePositionH = lightPV * worldPosition;
    vec3 lightspacePosition = lightspacePositionH.xyz / lightspacePositionH.w;
    vec2 shadowUVs = lightspacePosition.xy * 0.5f + vec2(0.5f);
    float smSample;
    if(any(lessThan(shadowUVs, vec2(0.0f, 0.0f))) || any(greaterThan(shadowUVs, vec2(1.0f, 1.0f)))) {
        // TGA won't let me change texture border color, without messing around in its internals
        return 1.0f;
    } else {
        smSample = texture(shadowMap, vec2(shadowUVs)).r;
    }
    //return texture(shadowMap, vec3(shadowUVs, lightspacePosition.z - bias));
    return (smSample > lightspacePosition.z - bias) ? 1.0 : 0.0;
}
