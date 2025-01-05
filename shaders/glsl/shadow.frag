#version 460
// This fragment shader is not strictly necessary, but TGA doesn't allow a render pass to be constructed without one.
// Also, they don't allow access to the depth buffers they create, so the easiest thing is to make an artificial one.

layout(location = 0) out float depth;

void main() {
    depth = gl_FragCoord.z;
}
