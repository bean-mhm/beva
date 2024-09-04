#version 450

layout(location = 0) in vec3 v_col;

layout(location = 0) out vec4 out_col;

void main()
{
    vec3 col = pow(v_col, vec3(1. / 2.2));
    out_col = vec4(col, 1);
}
