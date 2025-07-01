#version 450

// 通过两个三角形，绘制整个屏幕
// 注意顺序
vec2 output_position[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    gl_Position =vec4(output_position[gl_VertexIndex], 0.5, 1.0);
}
