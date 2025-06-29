#version 450

layout(push_constant) uniform PushConstants {
    int enableTexture;
} pc;

layout(binding = 1) uniform sampler2D texSampler;

layout(std140, binding = 2) uniform LightUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

layout(binding = 3) uniform sampler2D depthSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in float fragNa;
layout(location = 4) in vec3 fragKa;
layout(location = 5) in vec3 fragKd;
layout(location = 6) in vec3 fragKs;

layout(location = 0) out vec4 outColor;

void main() {
    // 计算光源空间坐标
    vec4 lightSpacePos = ubo.proj * ubo.view * ubo.model * vec4(fragPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5; // 将 x,y 转换到[0,1]区间

    float shadow = 0.0; // 光照范围之外
    if(projCoords.x > 0.0 && projCoords.x < 1.0 && projCoords.y > 0.0 && projCoords.y < 1.0) {
        float closestDepth = texture(depthSampler, projCoords.xy).r;
        float currentDepth = projCoords.z;
        // 阴影判断
        float bias = 0.001;
        shadow = currentDepth - bias > closestDepth ? 0.5 : 1.0; // 0.5表示阴影，1.0表示光照
    }


    // 根据推送常量决定是否采样纹理
    float depth = texture(depthSampler, fragTexCoord.xy).r;
    depth = (depth - 0.90) * 10.0;
    depth = pow(depth, 6.0);
    vec3 objectColor = pc.enableTexture == 1 ? texture(texSampler, fragTexCoord).rgb : vec3(depth, depth, depth);
    // 法线
    vec3 normal = normalize(fragNormal);
    // 视角方向
    vec3 viewDir = normalize(ubo.viewPos - fragPos);

    // 环境光强
    vec3 ambient = 0.2 * fragKa * ubo.lightColor;

    // 漫反射
    vec3 lightDir = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * fragKd * ubo.lightColor;

    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), fragNa);
    vec3 specular = 0.6 * spec * fragKs * ubo.lightColor;

    // 最终色彩
    vec3 result = (ambient + shadow * (diffuse + specular)) * objectColor;
    result = min(result, vec3(1.0));
    outColor = vec4(result, 1.0);

}
