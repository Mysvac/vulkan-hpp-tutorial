#version 450

struct Particle {
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(push_constant) uniform PushConstants {
    float deltaTime;
} pc;

layout(std140, binding = 0) buffer ParticleSSBO {
    Particle particles[ ];
};

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint index = gl_GlobalInvocationID.x;

    Particle particle = particles[index];

    particles[index].position = particle.position + particle.velocity.xy * pc.deltaTime;

    // Flip movement at window border
    if (abs(particles[index].position.x) >= 1.0) particles[index].velocity.x *= -1;
    if (abs(particles[index].position.y) >= 1.0) particles[index].velocity.y *= -1;
}
