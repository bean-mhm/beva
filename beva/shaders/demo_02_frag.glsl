#version 450

// uniforms
layout (binding = 0, rg32f) uniform readonly image2D img; // r=prev, g=current

// output from vertex shader
layout(location = 0) in vec2 v_texcoord;

// output from fragment shader
layout(location = 0) out vec4 out_col;

// colormap: https://www.shadertoy.com/view/MfjBDV
/*--------------------------------------------------------*/

// https://www.desmos.com/calculator/n4mfhffj1n
float colormap_expf(float x, float v)
{
    if (abs(v) < .0001) v = .0001;
    float p = pow(2., v);
    return (1. - pow(p, -x)) / (1. - 1. / p);
}

vec3 colormap(float x)
{
    float t = .18 * abs(x);
    if (x < 0.)
    {
        x = -x;
        t = -.37 - .14 * x;
    }
    
    // https://www.desmos.com/calculator/sdqk904uu9
    vec3 tone = 8. * vec3(
        cos(6.283 * t),
        cos(6.283 * (t - .3333)),
        cos(6.283 * (t - .6667))
    );
    
    x = smoothstep(0., 1., x);
    vec3 c = vec3(
        colormap_expf(x, tone.r),
        colormap_expf(x, tone.g),
        colormap_expf(x, tone.b)
    );
    
    c = mix(c, c + vec3(.03, 0, .03), smoothstep(.1, 0., x));
    
    return c;
}

// end of colormap
/*--------------------------------------------------------*/

bool icoord_in_bounds(ivec2 icoord, ivec2 ires)
{
    return
        icoord.x >= 0 &&
        icoord.y >= 0 &&
        icoord.x < ires.x &&
        icoord.y < ires.y;
}

float fetch(ivec2 icoord)
{
    if (!icoord_in_bounds(icoord, imageSize(img)))
    {
        return 0.;
    }
    return imageLoad(img, icoord).g;
}

void main()
{
    // manual bilinear interpolation

    vec2 coord = v_texcoord * vec2(imageSize(img));
    ivec2 icoord_tl = ivec2(floor(coord - .5));

    float x_fac = coord.x - (float(icoord_tl.x) + .5);
    float v = mix(
        mix(
            fetch(icoord_tl + ivec2(0, 0)),
            fetch(icoord_tl + ivec2(1, 0)),
            x_fac
        ),
        mix(
            fetch(icoord_tl + ivec2(0, 1)),
            fetch(icoord_tl + ivec2(1, 1)),
            x_fac
        ),
        coord.y - (float(icoord_tl.y) + .5)
    );

    out_col = vec4(pow(colormap(v), vec3(1. / 2.2)), 1);
}
