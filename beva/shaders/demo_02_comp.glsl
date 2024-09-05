#version 450

// specialization constants for the local invocation size
layout(local_size_x_id = 0) in;               
layout(local_size_y_id = 1) in;
layout(local_size_z_id = 2) in;

// push constants
layout(push_constant, std430) uniform pc {
    layout(offset = 0) ivec2 emitter_icoord; // wave source coordinates
    layout(offset = 8) uint global_frame_idx;
};

// input and output images (r=prev, g=current)
layout (binding = 0, rg32f) uniform readonly image2D input_img;
layout (binding = 1, rg32f) uniform writeonly image2D output_img;

/*--------------------------------------------------------*/

// minimum distance between two cells in the grid
const float wave_step = .01;
const float wave_step2 = wave_step * wave_step;

// grid dimensions
vec2 wave_dims = wave_step * vec2(imageSize(input_img) - 1);

// propagation speed in 2D
const float wave_speed = 10.;
const float wave_speed2 = wave_speed * wave_speed;

// minimum wavelength
// at least 8 steps needed for a perfectly smooth spherical wave
const float wave_min_wavelength = wave_step * sqrt(2.) * 8.;

// maximum frequency
const float wave_max_frequency = wave_speed / wave_min_wavelength;

// maximum timestep before blowing up
const float wave_max_dt = wave_step / (wave_speed * sqrt(2.));

// timestep
const float wave_dt = .99 * wave_max_dt;

// stiffness
// must be greater than or equal to 1 to function properly
const float wave_stiffness = 20.;

float sim_time = float(global_frame_idx) * wave_dt;

/*--------------------------------------------------------*/

const float PI = 3.141592653589793238462643383;
const float TAU = 6.283185307179586476925286767;

#define FUNC_REMAP(T) \
T remap(T v, float inp_start, float inp_end, float out_start, float out_end) \
{ \
    return out_start \
        + ((out_end - out_start) / (inp_end - inp_start)) * (v - inp_start); \
}

#define FUNC_REMAP_CLAMP(T) \
T remap_clamp( \
    T v, \
    float inp_start, \
    float inp_end, \
    float out_start, \
    float out_end \
) \
{ \
    T t = clamp((v - inp_start) / (inp_end - inp_start), 0., 1.); \
    return out_start + t * (out_end - out_start); \
}

#define FUNC_REMAP01(T) \
T remap01(T v, float inp_start, float inp_end) \
{ \
    return clamp((v - inp_start) / (inp_end - inp_start), 0., 1.); \
}

FUNC_REMAP(float)
FUNC_REMAP(vec2)
FUNC_REMAP(vec3)
FUNC_REMAP(vec4)

FUNC_REMAP_CLAMP(float)
FUNC_REMAP_CLAMP(vec2)
FUNC_REMAP_CLAMP(vec3)
FUNC_REMAP_CLAMP(vec4)

FUNC_REMAP01(float)
FUNC_REMAP01(vec2)
FUNC_REMAP01(vec3)
FUNC_REMAP01(vec4)

bool icoord_in_bounds(ivec2 icoord, ivec2 ires)
{
    return
        icoord.x >= 0 &&
        icoord.y >= 0 &&
        icoord.x < ires.x &&
        icoord.y < ires.y;
}

/*--------------------------------------------------------*/

float wave_update_val(ivec2 icoord, float curr)
{
    // initial pulse at the center
    if (icoord == imageSize(input_img) / 2)
    {
        float amp = remap_clamp(sim_time, 0., .04, 1., 0.);
        float freq = .3 * wave_max_frequency;
        if (amp > .001)
        {
            return amp * sin(TAU * sim_time * freq);
        }
    }

    if (icoord_in_bounds(emitter_icoord, imageSize(input_img)))
    {
        float emitter_prox = distance(vec2(icoord) + .5, vec2(emitter_icoord));
        emitter_prox /= max(
            float(imageSize(input_img).x),
            float(imageSize(input_img).y)
        );

        curr = mix(
            curr,
            curr + wave_dt * 30.,
            remap01(emitter_prox, .02, 0.)
        );
    }

    return curr;
}

void main()
{
    // global 1D index
    const uvec3 global_invocs = gl_NumWorkGroups * gl_WorkGroupSize;
    uint idx =
        gl_GlobalInvocationID.x
        + gl_GlobalInvocationID.y * global_invocs.x
        + gl_GlobalInvocationID.z * global_invocs.x * global_invocs.y;

    // discard out of bounds
    uvec2 sim_res = uvec2(imageSize(input_img));
    if (idx >= sim_res.x * sim_res.y)
    {
        return;
    }

    // current coordinates
    ivec2 icoord = ivec2(idx % sim_res.x, idx / sim_res.x);

    // clear values in the first frame
    if (global_frame_idx < 1)
    {
        imageStore(output_img, icoord, vec4(0));
        return;
    }

    // load the last values
    vec4 data = imageLoad(input_img, icoord);
    float prev = data.r;
    float curr = data.g;

    // fetch neighboring values
    float next_in_x = 0.;
    float prev_in_x = 0.;
    float next_in_y = 0.;
    float prev_in_y = 0.;
    if ((icoord.x + 1) < imageSize(input_img).x)
    {
        next_in_x = imageLoad(input_img, icoord + ivec2(1, 0)).g;
    }
    if ((icoord.x - 1) >= 0)
    {
        prev_in_x = imageLoad(input_img, icoord + ivec2(-1, 0)).g;
    }
    if ((icoord.y + 1) < imageSize(input_img).y)
    {
        next_in_y = imageLoad(input_img, icoord + ivec2(0, 1)).g;
    }
    if ((icoord.y - 1) >= 0)
    {
        prev_in_y = imageLoad(input_img, icoord + ivec2(0, -1)).g;
    }

    float grad_x = next_in_x - curr - curr + prev_in_x;
    float grad_y = next_in_y - curr - curr + prev_in_y;

    float acc = (grad_x + grad_y) * wave_speed2 / wave_step2;
    float vel = (curr - prev) / wave_dt;
    vel += (acc * wave_dt);
    vel *= pow(wave_stiffness, -wave_dt);

    prev = curr;
    curr += (vel * wave_dt);

    curr = wave_update_val(icoord, curr);

    imageStore(output_img, icoord, vec4(prev, curr, 0, 0));
}
