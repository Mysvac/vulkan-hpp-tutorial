#version 450

layout(std140, binding = 0) uniform LightUBO {
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

layout(input_attachment_index = 0, binding = 1) uniform subpassInput g_buffer[3];

layout(location = 0) out vec4 outColor;

void main() {
    vec3 pos = subpassLoad(g_buffer[0]).xyz;       // 片段位置
    vec3 color = subpassLoad(g_buffer[1]).rgb;   // 片段颜色
    vec3 normal = subpassLoad(g_buffer[2]).xyz; // 法线

    // 视角方向
    vec3 viewDir = normalize(ubo.viewPos - pos);

    // 环境光强
    vec3 ambient = 0.15 * ubo.lightColor;

    // 漫反射
    vec3 lightDir = normalize(ubo.lightPos - pos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * 0.8 * ubo.lightColor;

    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 512);
    vec3 specular = spec * 0.3 * ubo.lightColor;

    // 最终色彩
    vec3 result = (ambient + diffuse + specular) * color;
    result = min(result, vec3(1.0));
    outColor = vec4(result, 1.0);
}