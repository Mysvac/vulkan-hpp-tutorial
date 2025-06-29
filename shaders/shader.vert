#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in float inNa;
layout(location = 4) in vec3 inKa;
layout(location = 5) in vec3 inKd;
layout(location = 6) in vec3 inKs;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float fragNa;
layout(location = 4) out vec3 fragKa;
layout(location = 5) out vec3 fragKd;
layout(location = 6) out vec3 fragKs;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // 计算片元对应的世界坐标位置、法线和纹理坐标，并传递给着色器以进行 phong 光照计算
    fragPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
    fragNormal = mat3(ubo.model) * inNormal;
    fragTexCoord = inTexCoord;
    fragNa = inNa;
    fragKa = inKa;
    fragKd = inKd;
    fragKs = inKs;
}
