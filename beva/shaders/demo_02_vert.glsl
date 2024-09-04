#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 texcoord;

layout(location = 0) out vec3 v_col;
layout(location = 1) out vec2 v_texcoord;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1);
    v_col = col;
    v_texcoord = texcoord;
}
