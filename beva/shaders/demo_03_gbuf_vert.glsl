#version 450

// uniforms
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// input from vertex buffer
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

// output from vertex shader
layout(location = 0) out vec3 v_world_pos;
layout(location = 1) out vec3 v_world_normal;
layout(location = 2) out vec2 v_texcoord;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1);

    v_world_pos = (ubo.model * vec4(pos, 1)).xyz;
    v_world_normal = (transpose(inverse(ubo.model)) * vec4(normal, 0)).xyz;
    v_texcoord = texcoord;
}
