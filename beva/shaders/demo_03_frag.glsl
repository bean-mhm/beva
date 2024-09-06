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
layout(binding = 1) uniform sampler2D diffuse_metallic_tex;
layout(binding = 2) uniform sampler2D normal_roughness_tex;

// output from vertex shader
layout(location = 0) in vec3 v_world_pos;
layout(location = 1) in vec3 v_world_normal;
layout(location = 2) in vec2 v_texcoord;

// output from fragment shader
layout(location = 0) out vec4 out_col;

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
    vec4 data0 = texture(diffuse_metallic_tex, v_texcoord);
    vec4 data1 = texture(normal_roughness_tex, v_texcoord);

    vec3 diffuse = data0.rgb;
    float metallic = data0.a;
    
    vec3 world_normal = normalize(v_world_normal);
    
    // normal offset in tangent space (coming from the normal map)
    vec3 normal_offs = vec3(data1.rg * 2. - 1., 0);
    normal_offs = vec3(
        normal_offs.xy,
        sqrt(
            1.
            - (normal_offs.x * normal_offs.x)
            - (normal_offs.y * normal_offs.y)
        )
    );
    
    /*--------------------------------------------------------*/
    // NOTE: the following isn't the most performant or sensible way to get
    // tangent and bitangent vectors but it works for this demo. it is copied
    // and modified from the following link:
    // https://stackoverflow.com/a/5261402

    // gradient of world position in screen space
    vec3 world_pos_dx = dFdx(v_world_pos);
    vec3 world_pos_dy = dFdy(v_world_pos);

    // gradient of texture coordinates in screen space
    vec2 texcoord_dx = dFdx(v_texcoord);
    vec2 texcoord_dy = dFdy(v_texcoord);

    // initial tangent and bitangent
    vec3 tangent =
        normalize(texcoord_dy.y * world_pos_dx - texcoord_dx.y * world_pos_dy);
    vec3 bitangent =
        normalize(texcoord_dy.x * world_pos_dx - texcoord_dx.x * world_pos_dy);

    // get new tangent from the world normal
    tangent = cross(cross(world_normal, tangent), world_normal);
    tangent = normalize(tangent);

    // get new bitangent
    bitangent = -cross(cross(bitangent, world_normal), world_normal);
    bitangent = normalize(bitangent);
    
    // apply the normal offset (normal mapping)
    world_normal = mat3(tangent, bitangent, world_normal) * normal_offs;

    // end of goofy normal calculation
    /*--------------------------------------------------------*/

    float roughness = data1.b;

    vec3 col = vec3(0);
    if (render_mode == RENDER_MODE_LIT || render_mode == RENDER_MODE_DIFFUSE)
    {
        col = diffuse;
    }
    else if (render_mode == RENDER_MODE_NORMAL)
    {
        col = world_normal * .5 + .5;
    }
    else if (render_mode == RENDER_MODE_METALLIC_ROUGHNESS)
    {
        col = vec3(metallic, roughness, 0);
    }
    else if (render_mode == RENDER_MODE_DEPTH)
    {
        col = vec3(gl_FragCoord.z);
    }
    
    out_col = vec4(view_transform(col), 1);
}
