#version 450

layout(push_constant) uniform PushConstants {
    int enableTexture;
} pc;

layout(binding = 1) uniform sampler2D texSampler;

// layout(std140, binding = 2) uniform LightUBO 移除光源内容

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float fragNa;   // 无用
layout(location = 4) in vec3 fragKa;    // 无用
layout(location = 5) in vec3 fragKd;    // 无用
layout(location = 6) in vec3 fragKs;    // 无用

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outNormalDepth;

void main() {
    // 根据推送常量决定是否采样纹理
    vec3 objectColor = pc.enableTexture == 1 ? texture(texSampler, fragTexCoord).rgb : vec3(0.5, 0.5, 0.5);
    outColor = vec4(objectColor, 1.0);
    outPosition = vec4(fragPos, 1.0);
    outNormalDepth = vec4(fragNormal, 1.0); //第四位 深度信息此处不用
}