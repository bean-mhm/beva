#version 450

const int RENDER_MODE_LIT = 0;
const int RENDER_MODE_DIFFUSE = 1;
const int RENDER_MODE_NORMAL = 2;
const int RENDER_MODE_METALLIC_ROUGHNESS = 3;
const int RENDER_MODE_DEPTH = 4;
const int RENDER_MODE_POSITION_DERIVED = 5;

// push constants
layout(push_constant, std430) uniform pc {
    layout(offset = 0) mat4 inv_view_proj;
    layout(offset = 64) vec3 cam_pos;
    layout(offset = 76) float z_near;
    layout(offset = 80) float z_far;
    layout(offset = 84) int render_mode;
};

const float LIGHT_TYPE_AMBIENT = 0.;
const float LIGHT_TYPE_POINT = 1.;
const float LIGHT_TYPE_DIRECTIONAL = 2.;

struct Light
{
    // xyz = col, w = type
    vec4 data0;

    // xyz = pos_or_dir, w = useless
    vec4 data1;
};

// uniforms
layout(binding = 0) uniform sampler2D diffuse_metallic;
layout(binding = 1) uniform sampler2D normal_roughness;
layout(binding = 2) uniform sampler2D depth_sampler;
layout(std430, binding = 3) readonly buffer LightBuf {
   Light lights[];
};

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

float linearize_depth(float d)
{
    return z_near * z_far / (z_far + d * (z_near - z_far));
}

float safe_div(float a, float b)
{
    if (abs(b) < .001)
    {
        return a / (sign(b) * 1000.);
    }
    return a / b;
}

// https://github.com/pboechat/cook_torrance/blob/d139082e8d97c3722eb63be1c73bcff021b755f2/application/shaders/cook_torrance_textured.fs.glsl#L21
// slightly modified to remove NaNs
vec3 cook_torrance(
    vec3 materialDiffuseColor,
	vec3 materialSpecularColor,
	vec3 normal,
	vec3 lightDir,
	vec3 viewDir,
	vec3 lightColor,
    float roughness
)
{
	float NdotL = max(.0001, dot(normal, lightDir));
	float Rs = 0.;
	if (NdotL > .0001001) 
	{
		vec3 H = normalize(lightDir + viewDir);
		float NdotH = max(.0001, dot(normal, H));
		float NdotV = max(.0001, dot(normal, viewDir));
		float VdotH = max(.0001, dot(lightDir, H));

		// Fresnel reflectance
        const float F0 = .8;
		float F = pow(1. - VdotH, 5.);
		F *= (1. - F0);
		F += F0;

		// Microfacet distribution by Beckmann
		float m_squared = roughness * roughness;
		float r1 = safe_div(1., 4. * m_squared * pow(NdotH, 4.));
		float r2 = safe_div(NdotH * NdotH - 1., m_squared * NdotH * NdotH);
		float D = r1 * exp(r2);

		// Geometric shadowing
		float two_NdotH = 2. * NdotH;
		float g1 = (two_NdotH * NdotV) / VdotH;
		float g2 = (two_NdotH * NdotL) / VdotH;
		float G = min(1., min(g1, g2));

		Rs = (F * D * G) / (PI * NdotL * NdotV);
	}
	return
        (materialDiffuseColor * lightColor * NdotL)
        + (lightColor * materialSpecularColor * Rs);
}

void main()
{
    vec4 data0 = texture(diffuse_metallic, v_texcoord);
    vec4 data1 = texture(normal_roughness, v_texcoord);

    vec3 diffuse = pow(data0.rgb, vec3(2.2));
    float metallic = data0.a;
    vec3 world_normal = spherical_to_cartesian(
        vec3(1., PI * (data1.xy * 2. - 1.))
    );
    float roughness = data1.b;
    bool pixel_is_lit = data1.a > .5;

    float depth = texture(depth_sampler, v_texcoord).x;
    float depth_lin = linearize_depth(depth);

    // calculate world position
    // https://stackoverflow.com/questions/38938498/how-do-i-convert-gl-fragcoord-to-a-world-space-point-in-a-fragment-shader
    vec4 ndc = vec4(
        2. * gl_FragCoord.xy / vec2(textureSize(diffuse_metallic, 0)) - 1.,
        depth,
        1
    );
    vec4 clip = inv_view_proj * ndc;
    vec3 world_pos = clip.xyz / clip.w;

    vec3 col = vec3(0);
    if (render_mode == RENDER_MODE_LIT && pixel_is_lit)
    {
        for (int i = 0; i < lights.length(); i++)
        {
            Light light = lights[i];
            float light_type = light.data0.w;
            vec3 light_col = light.data0.xyz;
            vec3 light_pos_or_dir = light.data1.xyz;

            if (abs(light_type - LIGHT_TYPE_AMBIENT) < .1)
            {
                col += mix(diffuse * light_col, light_col, .2);
            }
            else if (abs(light_type - LIGHT_TYPE_POINT) < .1)
            {
                vec3 light_dir = light_pos_or_dir - world_pos;
                float light_dist = length(light_dir);
                light_dir /= light_dist;
                
                col += cook_torrance(
                    mix(diffuse, .3 * diffuse, metallic),
                    mix(vec3(.5), diffuse, metallic),
                    world_normal,
                    light_dir,
                    normalize(cam_pos - world_pos),
                    light_col / (light_dist * light_dist),
                    roughness
                );
            }
            else if (abs(light_type - LIGHT_TYPE_DIRECTIONAL) < .1)
            {
                col += cook_torrance(
                    mix(diffuse, .3 * diffuse, metallic),
                    mix(vec3(.5), diffuse, metallic),
                    world_normal,
                    light_pos_or_dir,
                    normalize(cam_pos - world_pos),
                    light_col,
                    roughness
                );
            }
        }
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
    else if (render_mode == RENDER_MODE_POSITION_DERIVED && pixel_is_lit)
    {
        col = max(world_pos, 0.);
    }
    
    // output linear values, the next pass will take care of the OETF for
    // outputting to the screen.
    out_col = vec4(col, 1);
}
