#version 450

layout(push_constant) uniform PushConstants {
    int enableTexture;
} pc;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 3) uniform texture2D texImage[2];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    if (pc.enableTexture < 0) {
        outColor = vec4(fragColor, 1.0);
    } else {
        outColor = texture(sampler2D(texImage[pc.enableTexture], texSampler), fragTexCoord);
    }
}
