#define S0_SIZE_OFFSET 0
#define S1_SIZE_OFFSET 1
#define S2_REFRACT_SIZE_OFFSET 2
#define S2_SPECULAR_SIZE_OFFSET 3
#define S2_DIFFUSE_SIZE_OFFSET 4
#define S2_LIGHT_SIZE_OFFSET 5

inline void atomic_add_global(volatile global float *source, const float operand) {
    union {
        unsigned int intVal;
        float floatVal;
    } newVal;
    union {
        unsigned int intVal;
        float floatVal;
    } prevVal;
 
    do {
        prevVal.floatVal = *source;
        newVal.floatVal = prevVal.floatVal + operand;
    } while (atomic_cmpxchg((volatile global unsigned int *)source, prevVal.intVal, newVal.intVal) != prevVal.intVal);
}

#define RAND_MAX 0xFFFFFFFFL

inline long rand(long * seed) {
    *seed = ((*seed) * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
    return ((*seed) >> 16) & RAND_MAX;
}

inline float randf(long * seed) {
    return convert_float(rand(seed)) / convert_float(RAND_MAX) - 0.5f;
}

inline float3 randf3(long * seed) {
    float3 ret;
    ret.x = randf(seed);
    ret.y = randf(seed);
    ret.z = randf(seed);
    return normalize(ret);
}

#define DIFFICULTY 1

#define DIFFUSE_SAMPLE (16 * DIFFICULTY)
#define LIGHT_SAMPLE (2 * DIFFICULTY)

#define GENERAL_THRESHOLD (5e-3f / float(DIFFICULTY))

#define LIGHT_SAMPLE_THRESHOLD (1.0f / DIFFUSE_SAMPLE / 2.0f)
#define DIFFUSE_SAMPLE_THRESHOLD (1.0f / DIFFUSE_SAMPLE * 1.5f)

float _single_intersect(float3 _start_p, float3 in_dir,
                        float3 pa, float3 pb, float3 pc) {
    float3 start_p = _start_p + 0.5f * in_dir;

    float3 a = in_dir;
    float3 b = pa - pb;
    float3 c = pa - pc;
    float3 t = pa - start_p;

    float x, m, n;
    float * result0 = &n;
    float * result1 = &m;
    float * result2 = &x;

    float4 line[3];
    line[0] = (float4)(a.x, b.x, c.x, t.x);
    line[1] = (float4)(a.y, b.y, c.y, t.y);
    line[2] = (float4)(a.z, b.z, c.z, t.z);

    float3 abs_a = fabs(a);

    if(abs_a.y > abs_a.x && abs_a.y > abs_a.z) {
        float4 tmp = line[0];
        line[0] = line[1];
        line[1] = tmp;
        float * tmpx = result0;
        result0 = result1;
        result1 = tmpx;
    } else if (abs_a.z > abs_a.x) {
        float4 tmp = line[0];
        line[0] = line[2];
        line[2] = tmp;
        float * tmpx = result0;
        result0 = result2;
        result2 = tmpx;
    }

    if(fabs(line[2].y) > fabs(line[1].y)) {
        float4 tmp = line[1];
        line[1] = line[2];
        line[2] = tmp;
        float * tmpx = result1;
        result1 = result2;
        result2 = tmpx;
    }

    line[1] += line[0] * (-line[1].s0 / line[0].s0);
    line[2] += line[0] * (-line[2].s0 / line[0].s0);
    line[2] += line[1] * (-line[2].s1 / line[1].s1);

    *result2 = line[2].w / line[2].z;
    *result1 = (line[1].w - line[1].z * (*result2)) / line[1].y;
    *result0 = (line[0].w - line[0].z * (*result2) - line[0].y * (*result1)) / line[0].x;

    // nan >= 0 returns false
    if(m >= 0 && m <= 1 && n >= 0 && n <= 1
       && m + n < 1 && x > 0) return x + 0.5f;

    return -44;

}

__kernel void naive_intersect(__global int * v_sizes,
                              __global unit_S0 * v_s0,
                              __global unit_S1 * v_s1,
                              __global float3 * scene_points,
                              __global int4 * scene_mesh,
                              const int scene_mesh_size) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S0_SIZE_OFFSET]) return;

    unit_S0 s0 = v_s0[global_id];

    int mat_id = -1;
    int geo_id = -1;
    float intersect_number = -42;
    int4 triangle;
    for(int i = 0 ; i < scene_mesh_size ; i += 1) {
        triangle = scene_mesh[i];
        float result = _single_intersect(s0.start_p, s0.in_dir,
                                         scene_points[triangle.x], 
                                         scene_points[triangle.y],
                                         scene_points[triangle.z]);
        if(result > 0 && (intersect_number < 0 || result < intersect_number)) {
            intersect_number = result;
            geo_id = i;
            mat_id = triangle.w;
        }
    }


    if(geo_id != -1) {
        int index = atomic_inc(v_sizes + S1_SIZE_OFFSET);
        v_s1[index].orig_id = s0.orig_id;
        v_s1[index].depth = s0.depth;
        v_s1[index].strength = s0.strength;
        v_s1[index].geometry = triangle;
        v_s1[index].start_p = s0.start_p;
        v_s1[index].in_dir = s0.in_dir;
        v_s1[index].intersect_number = intersect_number;
    }
}

__kernel void s1_run(__global int * v_sizes,
                     __global unit_S1 * v_s1,
                     __global float * v_result, // store final result
                     __global unit_S2_refract * v_s2_refract,
                     __global unit_S2_specular * v_s2_specular,
                     __global unit_S2_diffuse * v_s2_diffuse,
                     __global unit_S2_light * v_s2_light,
                     __global float3 * scene_points,
                     __global Material * v_materials) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S1_SIZE_OFFSET]) return;

    unit_S1 s1 = v_s1[global_id];

    float3 geo_a = scene_points[s1.geometry.x];
    float3 geo_b = scene_points[s1.geometry.y];
    float3 geo_c = scene_points[s1.geometry.z];
    Material mat = v_materials[s1.geometry.w];

    float3 normal = normalize(cross(geo_b - geo_a, geo_c - geo_a));

    float3 intersect_p = s1.start_p + s1.intersect_number * s1.in_dir;
    float3 result = s1.strength * mat.ambient;
    result *= fabs(dot(normal, s1.in_dir));

    __global float * target = v_result + s1.orig_id * 3;
    atomic_add_global(target, result.x);
    atomic_add_global(target+1, result.y);
    atomic_add_global(target+2, result.z);

    // refract
    float3 new_strength = s1.strength * (1.0f - mat.dissolve);
    if(length(new_strength) > GENERAL_THRESHOLD) {
        int index = atomic_inc(v_sizes + S2_REFRACT_SIZE_OFFSET);
        v_s2_refract[index].orig_id = s1.orig_id;
        v_s2_refract[index].depth = s1.depth;
        v_s2_refract[index].new_strength = new_strength;
        v_s2_refract[index].in_dir = s1.in_dir;
        v_s2_refract[index].normal = normal;
        v_s2_refract[index].intersect_p = intersect_p;
        v_s2_refract[index].optical_density = mat.optical_density;
    }

    // specular
    new_strength = s1.strength * mat.specular;
    if(length(new_strength) > GENERAL_THRESHOLD) {
        int index = atomic_inc(v_sizes + S2_SPECULAR_SIZE_OFFSET);
        v_s2_specular[index].orig_id = s1.orig_id;
        v_s2_specular[index].depth = s1.depth;
        v_s2_specular[index].new_strength = new_strength;
        v_s2_specular[index].in_dir = s1.in_dir;
        v_s2_specular[index].normal = normal;
        v_s2_specular[index].intersect_p = intersect_p;
    }

    // diffuse
    new_strength = s1.strength * mat.diffuse;
    float new_strength_length = length(new_strength);
    if(new_strength_length > DIFFUSE_SAMPLE_THRESHOLD) {
        int index = atomic_inc(v_sizes + S2_DIFFUSE_SIZE_OFFSET);
        v_s2_diffuse[index].orig_id = s1.orig_id;
        v_s2_diffuse[index].depth = s1.depth;
        v_s2_diffuse[index].new_strength = new_strength;
        v_s2_diffuse[index].in_dir = s1.in_dir;
        v_s2_diffuse[index].normal = normal;
        v_s2_diffuse[index].intersect_p = intersect_p;
    }

    // light, reuse diffuse strength
    new_strength /= (LIGHT_SAMPLE * DIFFUSE_SAMPLE);
    if(new_strength_length > LIGHT_SAMPLE_THRESHOLD) {
        int index = atomic_inc(v_sizes + S2_LIGHT_SIZE_OFFSET);
        v_s2_light[index].orig_id = s1.orig_id;
        v_s2_light[index].depth = s1.depth;
        v_s2_light[index].new_strength = new_strength;
        v_s2_light[index].in_dir = s1.in_dir;
        v_s2_light[index].normal = normal;
        v_s2_light[index].intersect_p = intersect_p;
    }
}

__kernel void s2_refract_run(__global int * v_sizes,
                             __global unit_S2_refract * v_s2_refract,
                             __global unit_S0 * v_s0) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_REFRACT_SIZE_OFFSET]) return;

    unit_S2_refract s2 = v_s2_refract[global_id];

    // compute refraction
    float cos_alpha = dot(s2.in_dir, -s2.normal);
    float reverse = 1.0f;
    if(cos_alpha < 0) reverse = -1;
    float alpha = acos(cos_alpha);
    float3 p = cos_alpha * s2.normal;
    float3 q = normalize(s2.in_dir + p);
    float sin_beta = sin(alpha) * pow(s2.optical_density, reverse);
    float beta = asin(sin_beta);

    float3 refraction_dir = -reverse * cos(beta) * s2.normal +
                            sin_beta * q;

    int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
    v_s0[index].orig_id = s2.orig_id;
    v_s0[index].depth = s2.depth + 1;
    v_s0[index].strength = s2.new_strength;
    v_s0[index].start_p = s2.intersect_p;
    v_s0[index].in_dir = refraction_dir;
}

__kernel void s2_specular_run(__global int * v_sizes,
                              __global unit_S2_specular * v_s2_specular,
                              __global unit_S0 * v_s0) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_SPECULAR_SIZE_OFFSET]) return;

    unit_S2_specular s2 = v_s2_specular[global_id];

    // compute reflection
    float dot_ = dot(s2.in_dir, s2.normal);
    float3 projection = dot_ * s2.normal;
    float3 reflection_dir = s2.in_dir - 2.0f * projection;

    int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
    v_s0[index].orig_id = s2.orig_id;
    v_s0[index].depth = s2.depth + 1;
    v_s0[index].strength = s2.new_strength;
    v_s0[index].start_p = s2.intersect_p;
    v_s0[index].in_dir = reflection_dir;
}

__kernel void s2_diffuse_run(__global int * v_sizes,
                             __global unit_S2_diffuse * v_s2_diffuse,
                             __global unit_S0 * v_s0,
                             __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_DIFFUSE_SIZE_OFFSET]) return;

    unit_S2_diffuse s2 = v_s2_diffuse[global_id];

    bool dir = dot(s2.in_dir, s2.normal) < 0;

    long rand_seed = v_seed[global_id] + global_id;

    for(int i = 0 ; i < DIFFUSE_SAMPLE ; i += 1) {
        float3 p = randf3(&rand_seed);
        float dot_normal = dot(p, s2.normal);
        if(dot_normal < 0) {
            dot_normal = -dot_normal;
            if(dir) p = -p;
        }
        float3 strength = s2.new_strength * dot_normal / DIFFUSE_SAMPLE;
        int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
        v_s0[index].orig_id = s2.orig_id;
        v_s0[index].depth = s2.depth + 1;
        v_s0[index].strength = strength;
        v_s0[index].start_p = s2.intersect_p;
        v_s0[index].in_dir = p;
    }
}

__kernel void s2_light_run(__global int * v_sizes,
                           __global unit_S2_light * v_s2_light,
                           __global unit_S0 * v_s0,
                           __global int4 * v_lights,
                           const int v_lights_size,
                           __global float3 * scene_points,
                           __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_LIGHT_SIZE_OFFSET]) return;

    unit_S2_light s2 = v_s2_light[global_id];

    bool dir = dot(s2.in_dir, s2.normal) < 0;

    long rand_seed = v_seed[global_id] + global_id;

    for(int i = 0 ; i < LIGHT_SAMPLE ; i += 1) {
        // random ray to light!
        int rand_light_index = rand(&rand_seed) % v_lights_size;
        int4 light = v_lights[rand_light_index];

        float3 pa = scene_points[light.x];
        float3 pb = scene_points[light.y];
        float3 pc = scene_points[light.z];

        float randx = randf(&rand_seed) + 0.5f; 
        float randy = randf(&rand_seed) + 0.5f; 
        if(randx + randy > 1) {
            randx = 1 - randx;
            randy = 1 - randy;
        }
        float3 p = pa + randx * (pb - pa) + randy * (pc - pa);

        float dot_ = dot(p, s2.normal);
        if((dot_ > 0) == dir) {
            int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
            v_s0[index].orig_id = s2.orig_id;
            v_s0[index].depth = s2.depth + 1;
            v_s0[index].strength = s2.new_strength * fabs(dot_);
            v_s0[index].start_p = s2.intersect_p;
            v_s0[index].in_dir = p;
        }
    }
}
