#define PCF_U_SAMPLES 4
#define PCF_V_SAMPLES 4

// can't use shadow sampler unfortunately, TGA does not expose the depth textures
float getShadowValue(mat4 lightPV, sampler2D shadowMap, vec4 worldPosition, float bias) {
    // TODO: slope dependent bias
    vec4 lightspacePositionH = lightPV * worldPosition;
    vec3 lightspacePosition = lightspacePositionH.xyz / lightspacePositionH.w;
    if(lightspacePosition.z >= 1.0f) {
        return 1.0f;
    }
    vec2 shadowUVs = lightspacePosition.xy * 0.5f + vec2(0.5f);
    float smSample = 0.0f;
    vec2 offsetRange = vec2(1.0f, 1.0f) / textureSize(shadowMap, 0);
    for(int i = 0; i < PCF_U_SAMPLES; ++i) {
        for(int j = 0; j < PCF_V_SAMPLES; ++j) {
            vec2 offsetUVs = vec2(mix(-offsetRange.x, offsetRange.x, float(i) / float(PCF_U_SAMPLES - 1)), mix(-offsetRange.y, offsetRange.y, float(j) / float(PCF_V_SAMPLES - 1)));
            offsetUVs = shadowUVs + offsetUVs;
            if(any(lessThan(offsetUVs, vec2(0.0f, 0.0f))) || any(greaterThan(offsetUVs, vec2(1.0f, 1.0f)))) {
                // TGA won't let me change texture border color, without messing around in its internals
                smSample += 1.0f;
            } else {
                smSample += (texture(shadowMap, offsetUVs).r > lightspacePosition.z - bias) ? 1.0f : 0.0f;
            }
        }
    }
    return smSample / float(PCF_U_SAMPLES * PCF_V_SAMPLES);
}
