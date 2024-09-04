#version 450

// uniforms
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// input from vertex buffer
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texcoord;

// input from instance buffer which is just a second vertex buffer with its
// input rate set to per instance. we could also use gl_InstanceIndex and index
// into a custom SSBO or whatever else to get instance data.
layout(location = 2) in vec3 instance_pos_offset;
layout(location = 3) in vec3 instance_col;

// output from vertex shader
layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec3 v_col;

void main()
{
    gl_Position =
        ubo.proj * ubo.view * ubo.model * vec4(pos + instance_pos_offset, 1);

    v_col = instance_col;
    v_texcoord = texcoord;
}
