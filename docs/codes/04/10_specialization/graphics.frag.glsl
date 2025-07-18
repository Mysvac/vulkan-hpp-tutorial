#version 450

// 提供默认值 0.0
layout (constant_id = 0) const float myColor = 0.0;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(texSampler, fragTexCoord).xy, myColor, 1.0);
}
