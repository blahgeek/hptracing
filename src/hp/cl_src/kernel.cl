#define S0_SIZE_OFFSET 0
#define S1_SIZE_OFFSET 1
#define S2_REFRACT_SIZE_OFFSET 2
#define S2_SPECULAR_SIZE_OFFSET 3
#define S2_DIFFUSE_SIZE_OFFSET 4
#define S2_LIGHT_SIZE_OFFSET 5

#define DATA_SIZE_OFFSET 6

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
#define PI 3.1415926535f

inline long rand(long * seed) {
    *seed = ((*seed) * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
    return ((*seed) >> 16) & RAND_MAX;
}

inline float randf(long * seed) {
    return convert_float(rand(seed)) / convert_float(RAND_MAX);
}

inline float3 randf3(long * seed) {
    float3 ret;
    float alpha = randf(seed) * PI;
    float beta = randf(seed) * PI;
    float cos_beta = cos(beta);
    ret.x = cos_beta * cos(alpha);
    ret.y = cos_beta * sin(alpha);
    ret.z = sin(beta);
    return ret;
}

#define GENERAL_THRESHOLD (1e-4f)

inline float _box_intersect_dimension(float p0, float p, float s) {
//    if(p == 0) return -1;
    return (s - p0) / p;
}

bool _box_intersect(float3 box_start, float3 box_end, float3 start_p, float3 in_dir) {
    // start_p inside box
    if(start_p.x >= box_start.x && start_p.x <= box_end.x &&
       start_p.y >= box_start.y && start_p.y <= box_end.y &&
       start_p.z >= box_start.z && start_p.z <= box_end.z) return true;
    float3 mins, maxs;

    float2 tmp;

    tmp.x = _box_intersect_dimension(start_p.x, in_dir.x, box_start.x);
    tmp.y = _box_intersect_dimension(start_p.x, in_dir.x, box_end.x);
    mins.s0 = fmin(tmp.x, tmp.y);
    maxs.s0 = fmax(tmp.x, tmp.y);
    tmp.x = _box_intersect_dimension(start_p.y, in_dir.y, box_start.y);
    tmp.y = _box_intersect_dimension(start_p.y, in_dir.y, box_end.y);
    mins.s1 = fmin(tmp.x, tmp.y);
    maxs.s1 = fmax(tmp.x, tmp.y);
    tmp.x = _box_intersect_dimension(start_p.z, in_dir.z, box_start.z);
    tmp.y = _box_intersect_dimension(start_p.z, in_dir.z, box_end.z);
    mins.s2 = fmin(tmp.x, tmp.y);
    maxs.s2 = fmax(tmp.x, tmp.y);

    float max_of_mins = fmax(fmax(mins.x, mins.y), mins.z);
    float min_of_maxs = fmin(fmin(maxs.x, maxs.y), maxs.z);

    if(min_of_maxs <= 0) return false;
    return max_of_mins <= min_of_maxs;
}

float _single_intersect(float3 start_p, float3 in_dir,
                        float3 pa, float3 pb, float3 pc) {
    #define EPSILON 1e-3f

    float3 e1 = pb - pa;
    float3 e2 = pc - pa;
    float3 P = cross(in_dir, e2);
    float det = dot(e1, P);
    if(det > EPSILON && det < EPSILON) return -1;

    float inv_det = 1.f / det;
    float3 T = start_p - pa;
    float u = dot(T, P) * inv_det;
    if(u < 0.f || u > 1.f) return -1;

    float3 Q = cross(T, e1);
    float v = dot(in_dir, Q) * inv_det;
    if(v < 0.f || u + v  > 1.f) return -1;

    float t = dot(e2, Q) * inv_det;
    if(t > 0) return t;

    return -1;

}

__kernel void kdtree_intersect(__global int * v_sizes,
                               __global unit_data * v_data,
                               __global int * v_s0,
                               __global int * v_s1,
                               __global float3 * scene_points,
                               __global int4 * scene_mesh,
                               __global int * v_kd_leaf_data,
                               __global KDTreeNodeHeader * v_kd_node_header,
                               const int kd_node_size) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S0_SIZE_OFFSET]) return;

    int this_id = v_s0[global_id];
    unit_data s0 = v_data[this_id];

    int geo_id = -1;
    float intersect_number = -42;

#define MAX_MATCH_DATA 512

    int match_data[MAX_MATCH_DATA];
    int match_data_size = 0;

    int node_index = 0;
    int come_from_child = 0;
    while(1) {
        int goto_child = 0;

        KDTreeNodeHeader node = v_kd_node_header[node_index];
        if(come_from_child == 0 && _box_intersect(node.box_start, node.box_end, s0.start_p, s0.in_dir)) {
            if(node.child < 0) {
                if(node.data >= 0 && match_data_size < MAX_MATCH_DATA)
                    match_data[match_data_size++] = node.data;
            }
            else 
                goto_child = 1;
        } 

        if(goto_child) {
            come_from_child = 0;
            node_index = node.child;
        } else {
            if(node.sibling >= 0) {
                come_from_child = 0;
                node_index = node.sibling;
            } else if(node.parent >= 0) {
                come_from_child = 1;
                node_index = node.parent;
            } else {
                // root
                break;
            }
        }
    }

    for(int i = 0 ; i < match_data_size ; i += 1) {
        __global int * data = v_kd_leaf_data + match_data[i];
        int data_size = data[0];
        for(int x = 0 ; x < data_size ; x += 1) {
            int triangle_id = data[x+1];
            int4 triangle = scene_mesh[triangle_id];
            float result = _single_intersect(s0.start_p, s0.in_dir,
                                             scene_points[triangle.x],
                                             scene_points[triangle.y],
                                             scene_points[triangle.z]);
            if(result > 0 && (intersect_number < 0 || result < intersect_number)) {
                intersect_number = result;
                geo_id = triangle_id;
            }
        }
    }

    if(geo_id != -1) {
        int index = atomic_inc(v_sizes + S1_SIZE_OFFSET);
        v_s1[index] = this_id;

        v_data[this_id].geometry = scene_mesh[geo_id];
        v_data[this_id].intersect_number = intersect_number;
    }
}

__kernel void naive_intersect(__global int * v_sizes,
                              __global unit_data * v_data,
                              __global int * v_s0,
                              __global int * v_s1,
                              __global float3 * scene_points,
                              __global int4 * scene_mesh,
                              const int scene_mesh_size) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S0_SIZE_OFFSET]) return;

    int this_id = v_s0[global_id];
    unit_data s0 = v_data[this_id];

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
        }
    }

    if(geo_id != -1) {
        int index = atomic_inc(v_sizes + S1_SIZE_OFFSET);
        v_s1[index] = this_id;

        v_data[this_id].geometry = scene_mesh[geo_id];
        v_data[this_id].intersect_number = intersect_number;
    }
}

__constant sampler_t sampler =
    CLK_NORMALIZED_COORDS_TRUE
    | CLK_ADDRESS_REPEAT
    | CLK_FILTER_LINEAR;

__kernel void s1_run(__global int * v_sizes,
                     __global unit_data * v_data,
                     __global int * v_s1,
                     __global float * v_result, // store final result
                     __global int * v_s2_refract,
                     __global int * v_s2_specular,
                     __global int * v_s2_diffuse,
                     __global int * v_s2_light,
                     __global float3 * scene_points,
                     __global float3 * scene_normals,
                     const int scene_normals_size,
                     __global float2 * scene_texcoords,
                     const int scene_texcoords_size,
                     __constant Material * v_materials,
                #ifdef CL_VERSION_1_2
                     __read_only image2d_array_t textures,
                #else
                     __global unsigned char * textures,
                #endif
                     const int disable_diffuse,
                     __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S1_SIZE_OFFSET]) return;

    int this_id = v_s1[global_id];
    unit_data s1 = v_data[this_id];

    float3 geo_a = scene_points[s1.geometry.x];
    float3 geo_b = scene_points[s1.geometry.y];
    float3 geo_c = scene_points[s1.geometry.z];
    Material mat = v_materials[s1.geometry.w];

    float3 intersect_p = s1.start_p + s1.intersect_number * s1.in_dir;

    float area1, area2, area3;

    float3 normal;
    if(scene_normals_size == 0) {
        normal = normalize(cross(geo_b - geo_a, geo_c - geo_a));
    } else {
        float3 f1 = geo_a - intersect_p;
        float3 f2 = geo_b - intersect_p;
        float3 f3 = geo_c - intersect_p;
        float area_all = length(cross(geo_b - geo_a, geo_c - geo_a));
        area1 = length(cross(f2, f3)) / area_all;
        area2 = length(cross(f1, f3)) / area_all;
        area3 = length(cross(f1, f2)) / area_all;
        normal = area1 * scene_normals[s1.geometry.x] + 
                 area2 * scene_normals[s1.geometry.y] +
                 area3 * scene_normals[s1.geometry.z];
    }

    float3 result = s1.strength * (mat.ambient + mat.emission);
    result *= fabs(dot(normal, s1.in_dir));

    __global float * target = v_result + s1.orig_id * 3;
    // We only have one orig_id per loop, this shouldn't be problem
    target[0] += result.x;
    target[1] += result.y;
    target[2] += result.z;
//    atomic_add_global(target, result.x);
//    atomic_add_global(target+1, result.y);
//    atomic_add_global(target+2, result.z);

    if(length(s1.strength) < GENERAL_THRESHOLD)
        return;

    v_data[this_id].intersect_p = intersect_p;
    v_data[this_id].normal = normal;


    long rand_seed = v_seed[global_id] + global_id;
    float rand_num = randf(&rand_seed);
    v_seed[global_id] = rand_seed;

    if(rand_num < mat.specular_possibility) {
        // specular!
        int index = atomic_inc(v_sizes + S2_SPECULAR_SIZE_OFFSET);
        v_s2_specular[index] = this_id;
        v_data[this_id].strength = s1.strength * mat.specular;
        v_data[this_id].shininess = mat.shininess;
        return;
    }

    if(rand_num < mat.refract_possibility) {
        // refract!
        int index = atomic_inc(v_sizes + S2_REFRACT_SIZE_OFFSET);
        v_s2_refract[index] = this_id;
        v_data[this_id].strength = s1.strength * (1.0f - mat.dissolve);
        v_data[this_id].optical_density = mat.optical_density;
        return;
    }

    float3 diffuse = mat.diffuse;
    if(mat.texture_id >= 0) {
        float2 uv;
        if(scene_texcoords_size == 0) {
            uv = normalize(intersect_p.xy);
        } else {
            uv = area1 * scene_texcoords[s1.geometry.x] +
                 area2 * scene_texcoords[s1.geometry.y] +
                 area3 * scene_texcoords[s1.geometry.z];
        }
    #ifdef CL_VERSION_1_2
        diffuse = read_imagef(textures, sampler, 
                              (float4)(uv.x, uv.y, mat.texture_id, 0)).xyz;
    #else
        __global unsigned char * texture_p = textures + 4 * 512 * 512 * mat.texture_id
                                             + (convert_int(uv.x * 512.f) * 512 + convert_int(uv.y * 512.f)) * 4;
        diffuse.x = convert_float(texture_p[0]) / 255.0f;
        diffuse.y = convert_float(texture_p[1]) / 255.0f;
        diffuse.z = convert_float(texture_p[2]) / 255.0f;
    #endif
    }

    if(rand_num < mat.diffuse_possibility && disable_diffuse == 0) {
        // diffuse!
        int index = atomic_inc(v_sizes + S2_DIFFUSE_SIZE_OFFSET);
        v_s2_diffuse[index] = this_id;
        v_data[this_id].strength = s1.strength * diffuse;
        return;
    }

    // rest is light
    int index = atomic_inc(v_sizes + S2_LIGHT_SIZE_OFFSET);
    v_s2_light[index] = this_id;
    v_data[this_id].strength = s1.strength * diffuse;

}

__kernel void s2_refract_run(__global int * v_sizes,
                             __global unit_data * v_data,
                             __global int * v_s2_refract,
                             __global int * v_s0) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_REFRACT_SIZE_OFFSET]) return;

    int this_id = v_s2_refract[global_id];
    unit_data s2 = v_data[this_id];

    // compute refraction
    float cos_alpha = dot(s2.in_dir, -s2.normal);
    float reverse = 1.0f;
    if(cos_alpha < 0) reverse = -1;
    float alpha = acos(cos_alpha);
    float3 p = cos_alpha * s2.normal;
    float3 q = normalize(s2.in_dir + p);
    float sin_beta = sin(alpha) * pow(s2.optical_density, -reverse);

    float3 final_dir; // may be inner reflect, may be refract
    if(sin_beta <= 1.f) {
        // refract
        float beta = asin(sin_beta);

        final_dir = -reverse * cos(beta) * s2.normal +
                    sin_beta * q;
    }
    else {
        final_dir = s2.in_dir + p + p;
    }

    int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
    v_s0[index] = this_id;

    v_data[this_id].start_p = s2.intersect_p + 0.5f * final_dir;
    v_data[this_id].in_dir = final_dir;
}

__kernel void s2_specular_run(__global int * v_sizes,
                              __global unit_data * v_data,
                              __global int * v_s2_specular,
                              __global int * v_s0,
                              __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_SPECULAR_SIZE_OFFSET]) return;

    int this_id = v_s2_specular[global_id];
    unit_data s2 = v_data[this_id];

    long rand_seed = v_seed[global_id] + global_id;
    float3 rand_v = randf3(&rand_seed) / 2.0 / s2.shininess;
    v_seed[global_id] = rand_seed;

    // compute reflection
    float dot_ = dot(s2.in_dir, s2.normal);
    float3 projection = dot_ * s2.normal;
    float3 reflection_dir = s2.in_dir - 2.0f * projection;

    reflection_dir = reflection_dir + rand_v;

    int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
    v_s0[index] = this_id;

    v_data[this_id].start_p = s2.intersect_p + 0.5f * reflection_dir;
    v_data[this_id].in_dir = reflection_dir;
}

__kernel void s2_diffuse_run(__global int * v_sizes,
                             __global unit_data * v_data,
                             __global int * v_s2_diffuse,
                             __global int * v_s0,
                             __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_DIFFUSE_SIZE_OFFSET]) return;

    int this_id = v_s2_diffuse[global_id];
    unit_data s2 = v_data[this_id];

    bool dir = dot(s2.in_dir, s2.normal) < 0;

    long rand_seed = v_seed[global_id] + global_id;

    float3 p = randf3(&rand_seed);
    float dot_normal = dot(p, s2.normal);
    if(dot_normal < 0) {
        dot_normal = -dot_normal;
        if(dir) p = -p;
    }
    float3 strength = s2.strength * dot_normal;

    int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
    v_s0[index] = this_id;

    v_data[this_id].strength = strength;
    v_data[this_id].start_p = s2.intersect_p + 0.5f * p;
    v_data[this_id].in_dir = p;
    v_seed[global_id] = rand_seed;
}

__kernel void s2_light_run(__global int * v_sizes,
                           __global unit_data * v_data,
                           __global int * v_s2_light,
                           __global int * v_s0,
                           __global int4 * v_lights,
                           const int v_lights_size,
                           __global float3 * scene_points,
                           __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_LIGHT_SIZE_OFFSET]) return;

    int this_id = v_s2_light[global_id];
    unit_data s2 = v_data[this_id];

    bool dir = dot(s2.in_dir, s2.normal) < 0;

    long rand_seed = v_seed[global_id] + global_id;

    int rand_light_index = rand(&rand_seed) % v_lights_size;
    int4 light = v_lights[rand_light_index];

    float3 pa = scene_points[light.x];
    float3 pb = scene_points[light.y];
    float3 pc = scene_points[light.z];

    float randx = randf(&rand_seed); 
    float randy = randf(&rand_seed); 
    if(randx + randy > 1) {
        randx = 1 - randx;
        randy = 1 - randy;
    }
    float3 point = pa + randx * (pb - pa) + randy * (pc - pa);
    float3 p = normalize(point - s2.intersect_p);

    float dot_ = dot(p, s2.normal);
    if((dot_ > 0) == dir) {
        int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);

        v_s0[index] = this_id;
        v_data[this_id].strength = s2.strength * fabs(dot_);
        v_data[this_id].start_p = s2.intersect_p + 0.5f * p;
        v_data[this_id].in_dir = p;
    }
    v_seed[global_id] = rand_seed;
}

__kernel void set_viewdirs(const float3 view_p,
                           const float3 top_dir,
                           const float3 right_dir,
                           const float width,
                           const float height,
                           const int sample_x,
                           const int sample_y,
                           __global unit_data * result) {
    int global_id_x = get_global_id(0);
    int global_id_y = get_global_id(1);

    if(global_id_x > sample_x || global_id_y > sample_y)
        return;

    float3 norm_dir = cross(top_dir, right_dir);
    norm_dir += top_dir * (convert_float(global_id_y) / convert_float(sample_y)
                           * height - height * 0.5f);
    norm_dir += right_dir * (convert_float(global_id_x) / convert_float(sample_x)
                             * width - width * 0.5f);

    int index = (sample_y - 1 - global_id_y) * sample_x + global_id_x;
    result[index].in_dir = normalize(norm_dir);
    result[index].orig_id = index;
    result[index].strength = (float3)(1, 1, 1);
    result[index].start_p = view_p;
}

__kernel void get_image(__global float * result_mem,
                        const int supersample_x,
                        const int supersample_y,
                        const int sample, 
                        const float brightness,
                        __write_only image2d_t img) {
    int global_id_x = get_global_id(0);
    int global_id_y = get_global_id(1);

    int width = get_image_width(img);
    int height = get_image_height(img);

    if(global_id_x >= width || global_id_y >= height)
        return;

    float4 pixel = (float4)(0, 0, 0, 0);
    for(int i = 0 ; i < supersample_x ; i += 1) {
        for(int j = 0 ; j < supersample_y ; j += 1) {
            int index = (global_id_y * supersample_y) * (width * supersample_x)
                        + global_id_x * supersample_x;
            pixel.x += result_mem[index * 3];
            pixel.y += result_mem[index * 3 + 1];
            pixel.z += result_mem[index * 3 + 2];
        }
    }
    pixel /= convert_float(supersample_x * supersample_y);
    pixel /= convert_float(sample);
    pixel *= brightness;

    pixel.x = 1.f - exp(-pixel.x);
    pixel.y = 1.f - exp(-pixel.y);
    pixel.z = 1.f - exp(-pixel.z);

    write_imagef(img, (int2)(global_id_x, global_id_y), pixel);
}
