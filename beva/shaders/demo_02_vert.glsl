#version 450

// input from vertex buffer
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texcoord;

// output from vertex shader
layout(location = 0) out vec2 v_texcoord;

void main()
{
    gl_Position = vec4(pos, 0, 1);
    v_texcoord = texcoord;
}
