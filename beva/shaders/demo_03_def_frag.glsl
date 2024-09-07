#version 450

const int RENDER_MODE_LIT = 0;
const int RENDER_MODE_DIFFUSE = 1;
const int RENDER_MODE_NORMAL = 2;
const int RENDER_MODE_METALLIC_ROUGHNESS = 3;
const int RENDER_MODE_DEPTH = 4;

// push constants
layout(push_constant, std430) uniform pc {
    layout(offset = 0) int render_mode;
};

// uniforms
layout(binding = 0) uniform sampler2D diffuse_metallic;
layout(binding = 1) uniform sampler2D normal_roughness;
layout(binding = 2) uniform sampler2D depth_sampler;

// output from vertex shader
layout(location = 0) in vec2 v_texcoord;

// output from fragment shader
layout(location = 0) out vec4 out_col;

const float PI = 3.141592653589793238462643383;
const float TAU = 6.283185307179586476925286767;
const float PI_OVER_2 = 1.570796326794896619231321692;
const float INV_PI = .318309886183790671537767527;
const float INV_TAU = .159154943091895335768883763;

// xyz = r, theta, phi
// https://gist.github.com/overdev/d0acea5729d43086b4841efb8f27c8e2
vec3 spherical_to_cartesian(vec3 s)
{
    return s.x * vec3(
        cos(s.z) * sin(s.y),
        sin(s.z) * sin(s.y),
        cos(s.y)
    );
}

vec3 view_transform(vec3 col)
{
    // eliminate negative values before using power functions
    col = max(col, 0.);

    // OETF (Linear BT.709 I-D65 to sRGB 2.2)
    col = pow(col, vec3(1. / 2.2));

    return col;
}

void main()
{
    vec4 data0 = texture(diffuse_metallic, v_texcoord);
    vec4 data1 = texture(normal_roughness, v_texcoord);
    float depth = texture(depth_sampler, v_texcoord).x;

    vec3 diffuse = pow(data0.rgb, vec3(2.2));
    float metallic = data0.a;
    vec3 world_normal = spherical_to_cartesian(
        vec3(1., PI * (data1.xy * 2. - 1.))
    );
    float roughness = data1.b;
    bool pixel_is_lit = data1.a > .5;

    vec3 col = vec3(0);
    if (render_mode == RENDER_MODE_LIT && pixel_is_lit)
    {
        const vec3 light_dir = normalize(vec3(-.1, -.1, .5));
        col = diffuse * max(0., dot(world_normal, light_dir));
    }
    else if (render_mode == RENDER_MODE_LIT && !pixel_is_lit)
    {
        col = diffuse;
    }
    else if (render_mode == RENDER_MODE_DIFFUSE && pixel_is_lit)
    {
        col = diffuse;
    }
    else if (render_mode == RENDER_MODE_NORMAL && pixel_is_lit)
    {
        col = world_normal * .5 + .5;
    }
    else if (render_mode == RENDER_MODE_METALLIC_ROUGHNESS && pixel_is_lit)
    {
        col = vec3(metallic, roughness, 0);
    }
    else if (render_mode == RENDER_MODE_DEPTH)
    {
        col = vec3(depth);
    }
    
    out_col = vec4(view_transform(col), 1);
}
