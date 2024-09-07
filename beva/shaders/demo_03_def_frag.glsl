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



/*-----------------------------------------------

flim - Filmic Color Transform

Input Color Space:   Linear BT.709 I-D65
Output Color Space:  Linear BT.709 I-D65 / sRGB 2.2 (depends on arguments)

Description:
  Experimental port of flim for GLSL/Shadertoy
  matching flim v1.1.0.

Author:
  Bean (beans_please on Shadertoy)

Original Repo:
  https://github.com/bean-mhm/flim

Original Shader:
  https://www.shadertoy.com/view/dd2yDz

-----------------------------------------------*/

// parameters

const float flim_pre_exposure = 4.3;
const vec3 flim_pre_formation_filter = vec3(1.);
const float flim_pre_formation_filter_strength = 0.;

const float flim_extended_gamut_red_scale = 1.05;
const float flim_extended_gamut_green_scale = 1.12;
const float flim_extended_gamut_blue_scale = 1.045;
const float flim_extended_gamut_red_rot = .5;
const float flim_extended_gamut_green_rot = 2.;
const float flim_extended_gamut_blue_rot = .1;
const float flim_extended_gamut_red_mul = 1.;
const float flim_extended_gamut_green_mul = 1.;
const float flim_extended_gamut_blue_mul = 1.;

const float flim_sigmoid_log2_min = -10.;
const float flim_sigmoid_log2_max = 22.;
const float flim_sigmoid_toe_x = .44;
const float flim_sigmoid_toe_y = .28;
const float flim_sigmoid_shoulder_x = .591;
const float flim_sigmoid_shoulder_y = .779;

const float flim_negative_film_exposure = 6.;
const float flim_negative_film_density = 5.;

const vec3 flim_print_backlight = vec3(1);
const float flim_print_film_exposure = 6.;
const float flim_print_film_density = 27.5;

const float flim_black_point = -1.; // -1 = auto
const vec3 flim_post_formation_filter = vec3(1);
const float flim_post_formation_filter_strength = 0.;
const float flim_midtone_saturation = 1.02;

// color space conversions
// the matrices below represent data in row-major, but GLSL matrices are in
// column-major, so we need to multiply a vec3 by a matrix rather than
// multiplying a matrix by a vec3.

const mat3 mat_bt2020_to_xyz = mat3(
     0.6369580483,  0.1446169036,  0.1688809752,
     0.2627002120,  0.6779980715,  0.0593017165,
     0.0000000000,  0.0280726930,  1.0609850577
);

const mat3 mat_xyz_to_bt2020 = mat3(
     1.7166511880, -0.3556707838, -0.2533662814,
    -0.6666843518,  1.6164812366,  0.0157685458,
     0.0176398574, -0.0427706133,  0.9421031212
);

const mat3 mat_bt709_to_xyz = mat3(
     0.4123907993,  0.3575843394,  0.1804807884,
     0.2126390059,  0.7151686788,  0.0721923154,
     0.0193308187,  0.1191947798,  0.9505321522
);

const mat3 mat_xyz_to_bt709 = mat3(
     3.2409699419, -1.5373831776, -0.4986107603,
    -0.9692436363,  1.8759675015,  0.0415550574,
     0.0556300797, -0.2039769589,  1.0569715142
);

const mat3 mat_dcip3_to_xyz = mat3(
     0.4451698156,  0.2771344092,  0.1722826698,
     0.2094916779,  0.7215952542,  0.0689130679,
     0.0000000000,  0.0470605601,  0.9073553944
);

const mat3 mat_xyz_to_dcip3 = mat3(
     2.7253940305, -1.0180030062, -0.4401631952,
    -0.7951680258,  1.6897320548,  0.0226471906,
     0.0412418914, -0.0876390192,  1.1009293786
);

vec3 oetf_pow(vec3 col, float power)
{
    return pow(col, vec3(1. / power));
}

vec3 eotf_pow(vec3 col, float power)
{
    return pow(col, vec3(power));
}

// flim's utility functions

float flim_wrap(float v, float start, float end)
{
    return start + mod(v - start, end - start);
}

float flim_remap(
    float v,
    float inp_start,
    float inp_end,
    float out_start,
    float out_end
)
{
    return out_start
        + ((out_end - out_start) / (inp_end - inp_start)) * (v - inp_start);
}

float flim_remap_clamp(
    float v,
    float inp_start,
    float inp_end,
    float out_start,
    float out_end
)
{
    float t = clamp((v - inp_start) / (inp_end - inp_start), 0., 1.);
    return out_start + t * (out_end - out_start);
}

float flim_remap01(
    float v,
    float inp_start,
    float inp_end
)
{
    return clamp((v - inp_start) / (inp_end - inp_start), 0., 1.);
}

vec3 flim_blender_rgb_to_hsv(vec3 rgb)
{
    float cmax, cmin, h, s, v, cdelta;
    vec3 c;

    cmax = max(rgb[0], max(rgb[1], rgb[2]));
    cmin = min(rgb[0], min(rgb[1], rgb[2]));
    cdelta = cmax - cmin;

    v = cmax;
    if (cmax != 0.)
    {
        s = cdelta / cmax;
    }
    else
    {
        s = 0.;
        h = 0.;
    }

    if (s == 0.)
    {
        h = 0.;
    }
    else
    {
        c = (vec3(cmax) - rgb.xyz) / cdelta;

        if (rgb.x == cmax)
        {
            h = c[2] - c[1];
        }
        else if (rgb.y == cmax)
        {
            h = 2. + c[0] - c[2];
        }
        else
        {
            h = 4. + c[1] - c[0];
        }

        h /= 6.;

        if (h < 0.)
        {
            h += 1.;
        }
    }

    return vec3(h, s, v);
}

vec3 flim_blender_hsv_to_rgb(vec3 hsv)
{
    float f, p, q, t, h, s, v;
    vec3 rgb;

    h = hsv[0];
    s = hsv[1];
    v = hsv[2];

    if (s == 0.)
    {
        rgb = vec3(v, v, v);
    }
    else
    {
        if (h == 1.)
        {
            h = 0.;
        }

        h *= 6.;
        int i = int(floor(h));
        f = h - float(i);
        rgb = vec3(f, f, f);
        p = v * (1. - s);
        q = v * (1. - (s * f));
        t = v * (1. - (s * (1. - f)));

        if (i == 0)
        {
            rgb = vec3(v, t, p);
        }
        else if (i == 1)
        {
            rgb = vec3(q, v, p);
        }
        else if (i == 2)
        {
            rgb = vec3(p, v, t);
        }
        else if (i == 3)
        {
            rgb = vec3(p, q, v);
        }
        else if (i == 4)
        {
            rgb = vec3(t, p, v);
        }
        else
        {
            rgb = vec3(v, p, q);
        }
    }

    return rgb;
}

vec3 flim_blender_hue_sat(vec3 col, float hue, float sat, float value)
{
    vec3 hsv = flim_blender_rgb_to_hsv(col);

    hsv[0] = fract(hsv[0] + hue + .5);
    hsv[1] = clamp(hsv[1] * sat, 0., 1.);
    hsv[2] = hsv[2] * value;

    return flim_blender_hsv_to_rgb(hsv);
}

float flim_rgb_avg(vec3 col)
{
    return (col.x + col.y + col.z) / 3.;
}

float flim_rgb_sum(vec3 col)
{
    return col.x + col.y + col.z;
}

float flim_rgb_max(vec3 col)
{
    return max(max(col.x, col.y), col.z);
}

float flim_rgb_min(vec3 col)
{
    return min(min(col.x, col.y), col.z);
}

vec3 flim_rgb_uniform_offset(vec3 col, float black_point, float white_point)
{
    float mono = flim_rgb_avg(col);
    float mono2 = flim_remap01(
        mono, black_point / 1000.,
        1. - (white_point / 1000.)
    );
    return col * (mono2 / mono);
}

vec3 flim_rgb_sweep(float hue)
{
    hue = flim_wrap(hue * 360., 0., 360.);

    vec3 col = vec3(1, 0, 0);
    col = mix(col, vec3(1, 1, 0), flim_remap01(hue, 0., 60.));
    col = mix(col, vec3(0, 1, 0), flim_remap01(hue, 60., 120.));
    col = mix(col, vec3(0, 1, 1), flim_remap01(hue, 120., 180.));
    col = mix(col, vec3(0, 0, 1), flim_remap01(hue, 180., 240.));
    col = mix(col, vec3(1, 0, 1), flim_remap01(hue, 240., 300.));
    col = mix(col, vec3(1, 0, 0), flim_remap01(hue, 300., 360.));
    
    return col;
}

vec3 flim_rgb_exposure_sweep_test(vec2 uv0to1)
{
    float hue = 1. - uv0to1.y;
    float exposure = flim_remap(uv0to1.x, 0., 1., -5., 10.);
    return flim_rgb_sweep(hue) * pow(2., exposure);
}

// https://www.desmos.com/calculator/khkztixyeu
float flim_super_sigmoid(
    float v,
    float toe_x,
    float toe_y,
    float shoulder_x,
    float shoulder_y
)
{
    // clip
    v = clamp(v, 0., 1.);
    toe_x = clamp(toe_x, 0., 1.);
    toe_y = clamp(toe_y, 0., 1.);
    shoulder_x = clamp(shoulder_x, 0., 1.);
    shoulder_y = clamp(shoulder_y, 0., 1.);

    // calculate straight line slope
    float slope = (shoulder_y - toe_y) / (shoulder_x - toe_x);

    // toe
    if (v < toe_x)
    {
        float toe_pow = slope * toe_x / toe_y;
        return toe_y * pow(v / toe_x, toe_pow);
    }

    // straight line
    if (v < shoulder_x)
    {
        float intercept = toe_y - (slope * toe_x);
        return slope * v + intercept;
    }

    // shoulder
    float shoulder_pow =
        -slope / (
            ((shoulder_x - 1.) / pow(1. - shoulder_x, 2.))
            * (1. - shoulder_y)
        );
    return
        (1. - pow(1. - (v - shoulder_x) / (1. - shoulder_x), shoulder_pow))
        * (1. - shoulder_y)
        + shoulder_y;
}

float flim_dye_mix_factor(float mono, float max_density)
{
    // log2 and map range
    float offset = pow(2., flim_sigmoid_log2_min);
    float fac = flim_remap01(
        log2(mono + offset),
        flim_sigmoid_log2_min,
        flim_sigmoid_log2_max
    );

    // calculate amount of exposure from 0 to 1
    fac = flim_super_sigmoid(
        fac,
        flim_sigmoid_toe_x,
        flim_sigmoid_toe_y,
        flim_sigmoid_shoulder_x,
        flim_sigmoid_shoulder_y
    );

    // calculate dye density
    fac *= max_density;

    // mix factor
    fac = pow(2., -fac);

    // clip and return
    return clamp(fac, 0., 1.);
}

vec3 flim_rgb_color_layer(
    vec3 col,
    vec3 sensitivity_tone,
    vec3 dye_tone,
    float max_density
)
{
    // normalize
    vec3 sensitivity_tone_norm =
        sensitivity_tone / flim_rgb_sum(sensitivity_tone);
    vec3 dye_tone_norm = dye_tone / flim_rgb_max(dye_tone);

    // dye mix factor
    float mono = dot(col, sensitivity_tone_norm);
    float mix_fac = flim_dye_mix_factor(mono, max_density);

    // dye mixing
    return mix(dye_tone_norm, vec3(1), mix_fac);
}

vec3 flim_rgb_develop(vec3 col, float exposure, float max_density)
{
    // exposure
    col *= pow(2., exposure);

    // blue-sensitive layer
    vec3 result = flim_rgb_color_layer(
        col,
        vec3(0, 0, 1),
        vec3(1, 1, 0),
        max_density
    );

    // green-sensitive layer
    result *= flim_rgb_color_layer(
        col,
        vec3(0, 1, 0),
        vec3(1, 0, 1),
        max_density
    );

    // red-sensitive layer
    result *= flim_rgb_color_layer(
        col,
        vec3(1, 0, 0),
        vec3(0, 1, 1),
        max_density
    );

    return result;
}

vec3 flim_gamut_extension_mat_row(
    float primary_hue,
    float scale,
    float rotate,
    float mul
)
{
    vec3 result = flim_blender_hsv_to_rgb(vec3(
        flim_wrap(primary_hue + (rotate / 360.), 0., 1.),
        1. / scale,
        1.
    ));
    result /= flim_rgb_sum(result);
    result *= mul;
    return result;
}

mat3 flim_gamut_extension_mat(
    float red_scale,
    float green_scale,
    float blue_scale,
    float red_rot,
    float green_rot,
    float blue_rot,
    float red_mul,
    float green_mul,
    float blue_mul
)
{
    mat3 m;
    m[0] = flim_gamut_extension_mat_row(
        0.,
        red_scale,
        red_rot,
        red_mul
    );
    m[1] = flim_gamut_extension_mat_row(
        1. / 3.,
        green_scale,
        green_rot,
        green_mul
    );
    m[2] = flim_gamut_extension_mat_row(
        2. / 3.,
        blue_scale,
        blue_rot,
        blue_mul
    );
    return m;
}

vec3 negative_and_print(vec3 col, vec3 backlight_ext)
{
    // develop negative
    col = flim_rgb_develop(
        col,
        flim_negative_film_exposure,
        flim_negative_film_density
    );

    // backlight
    col *= backlight_ext;

    // develop print
    col = flim_rgb_develop(
        col,
        flim_print_film_exposure,
        flim_print_film_density
    );

    return col;
}

// the flim transform

vec3 flim_transform(vec3 col, float exposure, bool convert_to_srgb)
{
    // eliminate negative values
    col = max(col, 0.);

    // pre-Exposure
    col *= pow(2., flim_pre_exposure + exposure);

    // clip very large values for float precision issues
    col = min(col, 5000.);

    // gamut extension matrix (Linear BT.709)
    mat3 extend_mat = flim_gamut_extension_mat(
        flim_extended_gamut_red_scale,
        flim_extended_gamut_green_scale,
        flim_extended_gamut_blue_scale,
        flim_extended_gamut_red_rot,
        flim_extended_gamut_green_rot,
        flim_extended_gamut_blue_rot,
        flim_extended_gamut_red_mul,
        flim_extended_gamut_green_mul,
        flim_extended_gamut_blue_mul
    );
    mat3 extend_mat_inv = inverse(extend_mat);

    // backlight in the extended gamut
    vec3 backlight_ext = flim_print_backlight * extend_mat;

    // upper limit in the print (highlight cap)
    const float big = 10000000.;
    vec3 white_cap = negative_and_print(vec3(big, big, big), backlight_ext);

    // pre-formation filter
    col = mix(
        col,
        col * flim_pre_formation_filter,
        flim_pre_formation_filter_strength
    );

    // convert to the extended gamut
    col *= extend_mat;

    // negative & print
    col = negative_and_print(col, backlight_ext);

    // convert from the extended gamut
    col *= extend_mat_inv;

    // eliminate negative values
    col = max(col, 0.);

    // white cap
    col /= white_cap;

    // black cap (-1 = auto)
    if (flim_black_point == -1.)
    {
        vec3 black_cap = negative_and_print(vec3(0.), backlight_ext);
        black_cap /= white_cap;
        col = flim_rgb_uniform_offset(
            col,
            flim_rgb_avg(black_cap) * 1000.,
            0.
        );
    }
    else
    {
        col = flim_rgb_uniform_offset(col, flim_black_point, 0.);
    }

    // post-formation filter
    col = mix(
        col,
        col * flim_post_formation_filter,
        flim_post_formation_filter_strength
    );

    // clip
    col = clamp(col, 0., 1.);

    // midtone saturation
    float mono = flim_rgb_avg(col);
    float mix_fac =
        (mono < .5)
        ? flim_remap01(mono, .05, .5)
        : flim_remap01(mono, .95, .5);
    col = mix(
        col,
        flim_blender_hue_sat(col, .5, flim_midtone_saturation, 1.),
        mix_fac
    );

    // clip
    col = clamp(col, 0., 1.);

    // OETF
    if (convert_to_srgb)
    {
        col = oetf_pow(col, 2.2);
    }

    return col;
}

/*____________________ end ____________________*/



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

vec3 native_view_transform(vec3 col)
{
    // eliminate negative values before using power functions
    col = max(col, 0.);

    // OETF (Linear BT.709 I-D65 to sRGB 2.2)
    col = pow(col, vec3(1. / 2.2));

    return col;
}

// https://github.com/pboechat/cook_torrance/blob/d139082e8d97c3722eb63be1c73bcff021b755f2/application/shaders/cook_torrance_textured.fs.glsl#L21
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
	float NdotL = max(0., dot(normal, lightDir));
	float Rs = 0.0;
	if (NdotL > 0.) 
	{
		vec3 H = normalize(lightDir + viewDir);
		float NdotH = max(0., dot(normal, H));
		float NdotV = max(0., dot(normal, viewDir));
		float VdotH = max(0., dot(lightDir, H));

		// Fresnel reflectance
        const float F0 = 0.8;
		float F = pow(1.0 - VdotH, 5.0);
		F *= (1.0 - F0);
		F += F0;

		// Microfacet distribution by Beckmann
		float m_squared = roughness * roughness;
		float r1 = 1.0 / (4.0 * m_squared * pow(NdotH, 4.0));
		float r2 = (NdotH * NdotH - 1.0) / (m_squared * NdotH * NdotH);
		float D = r1 * exp(r2);

		// Geometric shadowing
		float two_NdotH = 2.0 * NdotH;
		float g1 = (two_NdotH * NdotV) / VdotH;
		float g2 = (two_NdotH * NdotL) / VdotH;
		float G = min(1.0, min(g1, g2));

		Rs = (F * D * G) / (PI * NdotL * NdotV);
	}
	return materialDiffuseColor * lightColor * NdotL + lightColor * materialSpecularColor * Rs;
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
        col = flim_transform(col, 0., false);
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
    
    out_col = vec4(native_view_transform(col), 1);
}
