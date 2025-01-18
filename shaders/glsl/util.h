float linearizeDepth(float depth, float zNear, float zFar) {
    return zNear * zFar / (zFar + depth * (zNear - zFar));
}
