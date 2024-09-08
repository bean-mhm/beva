#version 450

// uniforms
layout(binding = 1) uniform sampler2D diffuse_metallic_tex;
layout(binding = 2) uniform sampler2D normal_roughness_tex;

// output from vertex shader
layout(location = 0) in vec3 v_world_pos;
layout(location = 1) in vec3 v_world_normal;
layout(location = 2) in vec2 v_texcoord;

// output from fragment shader
layout(location = 0) out vec4 out_diffuse_metallic;
layout(location = 1) out vec4 out_normal_roughness;

const float PI = 3.141592653589793238462643383;
const float TAU = 6.283185307179586476925286767;
const float PI_OVER_2 = 1.570796326794896619231321692;
const float INV_PI = .318309886183790671537767527;
const float INV_TAU = .159154943091895335768883763;

// xyz = r, theta, phi
// https://gist.github.com/overdev/d0acea5729d43086b4841efb8f27c8e2
vec3 cartesian_to_spherical(vec3 p)
{
    return vec3(
        length(p),
        atan(sqrt(p.x * p.x + p.y * p.y), p.z),
        atan(p.y, p.x)
    );
}

void main()
{
    vec4 data0 = texture(diffuse_metallic_tex, v_texcoord);
    vec4 data1 = texture(normal_roughness_tex, v_texcoord);

    vec3 diffuse_srgb = data0.rgb;
    float metallic = data0.a;
    
    vec3 world_normal = normalize(v_world_normal);
    
    // normal in tangent space (coming from the normal map)
    vec3 normal_offs = vec3(data1.rg * 2. - 1., 0);
    normal_offs.z = sqrt(
        1.
        - (normal_offs.x * normal_offs.x)
        - (normal_offs.y * normal_offs.y)
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

    // end of normal calculation
    /*--------------------------------------------------------*/

    float roughness = data1.b;

    out_diffuse_metallic = vec4(diffuse_srgb, metallic);
    out_normal_roughness = vec4(
        clamp(
            cartesian_to_spherical(world_normal).yz * INV_PI * .5 + .5,
            0., 1.
        ),
        roughness,
        1
    );
}
