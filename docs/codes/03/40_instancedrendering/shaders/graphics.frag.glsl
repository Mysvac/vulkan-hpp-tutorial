#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint enableTexture;

layout(location = 0) out vec4 outColor;

void main() {
    if (enableTexture > 0) {
        outColor = texture(texSampler, fragTexCoord);
    } else {
        outColor = vec4(fragColor, 1.0);
    }
}