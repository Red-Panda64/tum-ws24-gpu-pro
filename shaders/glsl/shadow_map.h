// can't use shadow sampler unfortunately, TGA does not expose the depth textures
float getShadowValue(mat4 lightPV, vec3 lightDir, sampler2D shadowMap, vec4 worldPosition, vec3 worldNormal, float bias) {
    bias = max(bias * 5.0 * (1.0 - abs(dot(lightDir, worldNormal))), bias);

    vec4 lightspacePositionH = lightPV * worldPosition;
    vec3 lightspacePosition = lightspacePositionH.xyz / lightspacePositionH.w;
    if(lightspacePosition.z >= 1.0f) {
        return 1.0f;
    }
    vec2 shadowUVs = lightspacePosition.xy * 0.5f + vec2(0.5f);
    vec2 texelIdx = shadowUVs * textureSize(shadowMap, 0);
    vec2 texelFract = fract(texelIdx);
    shadowUVs = floor(texelIdx) / textureSize(shadowMap, 0);
    vec4 pcfSamples = textureGather(shadowMap, shadowUVs);
    vec4 testResults = vec4(greaterThan(pcfSamples, vec4(lightspacePosition.z - bias)));
    return mix(mix(testResults.x, testResults.y, texelFract.x), mix(testResults.w, testResults.z, texelFract.x), 1.0 - texelFract.y);
}
