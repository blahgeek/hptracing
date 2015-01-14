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

//#define DIFFICULTY 7
//
//#define DIFFUSE_SAMPLE (16 * DIFFICULTY)
//#define LIGHT_SAMPLE (2 * DIFFICULTY)
//
//#define DIFFUSE_SAMPLE_DIV (2 * DIFFICULTY)
//
//#define GENERAL_THRESHOLD (5e-3f / convert_float(DIFFICULTY))
//
//#define LIGHT_SAMPLE_THRESHOLD (1.0f / DIFFUSE_SAMPLE_DIV / 2.0f)
//#define DIFFUSE_SAMPLE_THRESHOLD (1.0f / DIFFUSE_SAMPLE_DIV * 1.5f)
#define GENERAL_THRESHOLD (1e-3f)

inline float _box_intersect_dimension(float p0, float p, float s) {
    if(p == 0) return -1;
    return (s - p0) / p;
}

#define SWAP_F(X, Y) \
    do { \
        float __tmp = (X); \
        (X) = (Y); \
        (Y) = __tmp; \
    } while(0) 

//bool
//intersection(box b, ray r)
//{
//  double tx1 = (b.min.x - r.x0.x)*r.n_inv.x;
//  double tx2 = (b.max.x - r.x0.x)*r.n_inv.x;
// 
//  double tmin = min(tx1, tx2);
//  double tmax = max(tx1, tx2);
// 
//  double ty1 = (b.min.y - r.x0.y)*r.n_inv.y;
//  double ty2 = (b.max.y - r.x0.y)*r.n_inv.y;
// 
//  tmin = max(tmin, min(ty1, ty2));
//  tmax = min(tmax, max(ty1, ty2));
// 
//  return tmax >= tmin;
//}
bool _box_intersect(float3 box_start, float3 box_end, float3 start_p, float3 in_dir) {
    // start_p inside box
    if(start_p.x >= box_start.x && start_p.x <= box_end.x &&
       start_p.y >= box_start.y && start_p.y <= box_end.y &&
       start_p.z >= box_start.z && start_p.z <= box_end.z) return true;
    float3 mins, maxs;

    mins.s0 = _box_intersect_dimension(start_p.x, in_dir.x, box_start.x);
    maxs.s0 = _box_intersect_dimension(start_p.x, in_dir.x, box_end.x);
    if(mins.s0 > maxs.s0) SWAP_F(mins.s0, maxs.s0);
    mins.s1 = _box_intersect_dimension(start_p.y, in_dir.y, box_start.y);
    maxs.s1 = _box_intersect_dimension(start_p.y, in_dir.y, box_end.y);
    if(mins.s1 > maxs.s1) SWAP_F(mins.s1, maxs.s1);
    mins.s2 = _box_intersect_dimension(start_p.z, in_dir.z, box_start.z);
    maxs.s2 = _box_intersect_dimension(start_p.z, in_dir.z, box_end.z);
    if(mins.s2 > maxs.s2) SWAP_F(mins.s2, maxs.s2);

    float max_of_mins = fmax(fmax(mins.x, mins.y), mins.z);
    float min_of_maxs = maxs.s0;

    if(min_of_maxs < 0 || (maxs.s1 >= 0 && maxs.s1 < min_of_maxs))
        min_of_maxs = maxs.s1;
    if(min_of_maxs < 0 || (maxs.s2 >= 0 && maxs.s2 < min_of_maxs))
        min_of_maxs = maxs.s2;
    return max_of_mins <= min_of_maxs;
}

float _single_intersect(float3 _start_p, float3 in_dir,
                        float3 pa, float3 pb, float3 pc) {
    float3 start_p = _start_p + 0.5f * in_dir;

//    #define EPSILON 1e-3f
//
//    float3 e1 = pb - pa;
//    float3 e2 = pc - pa;
//    float3 P = cross(in_dir, e2);
//    float det = dot(e1, P);
//    if(det > EPSILON && det < EPSILON) return -1;
//
//    float inv_det = 1.f / det;
//    float3 T = start_p - pa;
//    float u = dot(T, P) * inv_det;
//    if(u < 0.f || u > 1.f) return -1;
//
//    float3 Q = cross(T, e1);
//    float v = dot(in_dir, Q) * inv_det;
//    if(v < 0.f || u + v  > 1.f) return -1;
//
//    float t = dot(e2, Q) * inv_det;
//    if(t > 0.5) return t;
//
//    return -1;
//
    float3 a = in_dir;
    float3 b = pa - pb;
    float3 c = pa - pc;
    float3 t = pa - start_p;

    float x, m, n;

    float4 line[3];
    line[0] = (float4)(a.x, b.x, c.x, t.x);
    line[1] = (float4)(a.y, b.y, c.y, t.y);
    line[2] = (float4)(a.z, b.z, c.z, t.z);

    float3 abs_a = fabs(a);

    if(abs_a.y > abs_a.x && abs_a.y > abs_a.z) {
        float4 tmp = line[0];
        line[0] = line[1];
        line[1] = tmp;
    } else if (abs_a.z > abs_a.x) {
        float4 tmp = line[0];
        line[0] = line[2];
        line[2] = tmp;
    }

    if(fabs(line[2].y) > fabs(line[1].y)) {
        float4 tmp = line[1];
        line[1] = line[2];
        line[2] = tmp;
    }

    line[1] += line[0] * (-line[1].s0 / line[0].s0);
    line[2] += line[0] * (-line[2].s0 / line[0].s0);
    line[2] += line[1] * (-line[2].s1 / line[1].s1);

    n = line[2].w / line[2].z;
    m = (line[1].w - line[1].z * n) / line[1].y;
    x = (line[0].w - line[0].z * n - line[0].y * m) / line[0].x;

    // nan >= 0 returns false
    if(m >= 0 && m <= 1 && n >= 0 && n <= 1
       && m + n < 1 && x > 0) 
        return x + 0.5f;

    return -44;

}

__kernel void kdtree_intersect(__global int * v_sizes,
                               __global unit_data * v_data,
                               __global int * v_s0,
                               __global int * v_s1,
                               __global float3 * scene_points,
                               __global int4 * scene_mesh,
                               __constant int * v_kd_leaf_data,
                               __constant KDTreeNodeHeader * v_kd_node_header,
                               const int kd_node_size) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S0_SIZE_OFFSET]) return;

    int this_id = v_s0[global_id];
    unit_data s0 = v_data[this_id];

    int match_datas[256];
    int match_data_size = 0;

    int node_index = 0;
    int come_from_child = 0;
    while(node_index < kd_node_size) {
        int goto_child = 0;

        KDTreeNodeHeader node = v_kd_node_header[node_index];
        if(come_from_child == 0) {
            if(_box_intersect(node.box_start, node.box_end, s0.start_p, s0.in_dir)) {
                if(node.child < 0) {
                    if(match_data_size < 256 && node.data >= 0)
                        match_datas[match_data_size++] = node.data;
                }
                else 
                    goto_child = 1;
            }
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

    int geo_id = -1;
    float intersect_number = -42;

    for(int i = 0 ; i < match_data_size ; i += 1) {
        __constant int * data = v_kd_leaf_data + match_datas[i];
        int data_size = data[0];
        for(int x = 0 ; x < data_size ; x += 1) {
            int triangle_id = data[1+x];
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

__kernel void s1_run(__global int * v_sizes,
                     __global unit_data * v_data,
                     __global int * v_s1,
                     __global float * v_result, // store final result
                     __global int * v_s2_refract,
                     __global int * v_s2_specular,
                     __global int * v_s2_diffuse,
                     __global int * v_s2_light,
                     __global float3 * scene_points,
                     __constant Material * v_materials,
                     __global long * v_seed) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S1_SIZE_OFFSET]) return;

    int this_id = v_s1[global_id];
    unit_data s1 = v_data[this_id];

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

    v_data[this_id].intersect_p = intersect_p;
    v_data[this_id].normal = normal;

    // russia roulette
//    float specular_length = length(mat.specular);
//    float diffuse_length = length(mat.diffuse);
//    float refract_length = 1.0f - mat.dissolve;
//    float sum = specular_length + diffuse_length + refract_length + 1e-4;
//
//    float specular_possibility = specular_length / sum;

    long rand_seed = v_seed[global_id] + global_id;
    float rand_num = randf(&rand_seed) + 0.5f;
//    long rand_num = 0.f;
    v_seed[global_id] = rand_seed;

    if(rand_num < mat.specular_possibility) {
        // specular!
        int index = atomic_inc(v_sizes + S2_SPECULAR_SIZE_OFFSET);
        v_s2_specular[index] = this_id;
        v_data[this_id].strength = s1.strength * mat.specular;
        return;
    }
//    float refract_possibility = refract_length / sum + specular_possibility;

    if(rand_num < mat.refract_possibility) {
        // refract!
        int index = atomic_inc(v_sizes + S2_REFRACT_SIZE_OFFSET);
        v_s2_refract[index] = this_id;
        v_data[this_id].strength = s1.strength * (1.0f - mat.dissolve);
        v_data[this_id].optical_density = mat.optical_density;
        return;
    }
//    float diffuse_possibility = diffuse_length / 2.f / sum + refract_possibility;
//    float diffuse_possibility = diffuse_length / sum + refract_possibility;

    if(rand_num < mat.diffuse_possibility) {
        // diffuse!
        int index = atomic_inc(v_sizes + S2_DIFFUSE_SIZE_OFFSET);
        v_s2_diffuse[index] = this_id;
        v_data[this_id].strength = s1.strength * mat.diffuse;
        return;
    }

    // rest is light
    int index = atomic_inc(v_sizes + S2_LIGHT_SIZE_OFFSET);
    v_s2_light[index] = this_id;
    v_data[this_id].strength = s1.strength * mat.diffuse;

//    // refract
//    float3 new_strength = s1.strength * (1.0f - mat.dissolve);
//    if(length(new_strength) > GENERAL_THRESHOLD) {
//        int index = atomic_inc(v_sizes + S2_REFRACT_SIZE_OFFSET);
//        v_s2_refract[index] = this_id;
//        v_data[this_id].new_strength_refract = new_strength;
//        v_data[this_id].optical_density = mat.optical_density;
//    }

//    // specular
//    new_strength = s1.strength * mat.specular;
//    if(length(new_strength) > GENERAL_THRESHOLD) {
//        int index = atomic_inc(v_sizes + S2_SPECULAR_SIZE_OFFSET);
//        v_s2_specular[index] = this_id;
//        v_data[this_id].new_strength_specular = new_strength;
//    }

//    // diffuse
//    new_strength = s1.strength * mat.diffuse;
//    float new_strength_length = length(new_strength);
//    if(new_strength_length > DIFFUSE_SAMPLE_THRESHOLD) {
//        int index = atomic_inc(v_sizes + S2_DIFFUSE_SIZE_OFFSET);
//        v_s2_diffuse[index] = this_id;
//        v_data[this_id].new_strength_diffuse = new_strength;
//    }

//    // light, reuse diffuse strength
//    new_strength /= convert_float(DIFFUSE_SAMPLE);
//    if(new_strength_length > LIGHT_SAMPLE_THRESHOLD) {
//        int index = atomic_inc(v_sizes + S2_LIGHT_SIZE_OFFSET);
//        v_s2_light[index] = this_id;
//        v_data[this_id].new_strength_light = new_strength;
//    }
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
//    int new_id = atomic_inc(v_sizes + DATA_SIZE_OFFSET);
    v_s0[index] = this_id;

//    v_data[new_id].orig_id = s2.orig_id;
//    v_data[new_id].strength = s2.new_strength_refract;
    v_data[this_id].start_p = s2.intersect_p;
    v_data[this_id].in_dir = final_dir;
}

__kernel void s2_specular_run(__global int * v_sizes,
                              __global unit_data * v_data,
                              __global int * v_s2_specular,
                              __global int * v_s0) {
    int global_id = get_global_id(0);
    if(global_id >= v_sizes[S2_SPECULAR_SIZE_OFFSET]) return;

    int this_id = v_s2_specular[global_id];
    unit_data s2 = v_data[this_id];

    // compute reflection
    float dot_ = dot(s2.in_dir, s2.normal);
    float3 projection = dot_ * s2.normal;
    float3 reflection_dir = s2.in_dir - 2.0f * projection;

    int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
//    int new_id = atomic_inc(v_sizes + DATA_SIZE_OFFSET);
    v_s0[index] = this_id;

//    v_data[new_id].orig_id = s2.orig_id;
//    v_data[new_id].strength = s2.new_strength_specular;
    v_data[this_id].start_p = s2.intersect_p;
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

//    for(int i = 0 ; i < DIFFUSE_SAMPLE ; i += 1) {
        float3 p = randf3(&rand_seed);
        float dot_normal = dot(p, s2.normal);
        if(dot_normal < 0) {
            dot_normal = -dot_normal;
            if(dir) p = -p;
        }
//        float3 strength = s2.new_strength_diffuse * dot_normal / convert_float(DIFFUSE_SAMPLE);
        float3 strength = s2.strength * dot_normal;

        int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
//        int new_id = atomic_inc(v_sizes + DATA_SIZE_OFFSET);
        v_s0[index] = this_id;

//        v_data[new_id].orig_id = s2.orig_id;
        v_data[this_id].strength = strength;
        v_data[this_id].start_p = s2.intersect_p;
        v_data[this_id].in_dir = p;
//    }
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

//    for(int i = 0 ; i < LIGHT_SAMPLE ; i += 1) {
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
        float3 point = pa + randx * (pb - pa) + randy * (pc - pa);
        float3 p = normalize(point - s2.intersect_p);

        float dot_ = dot(p, s2.normal);
        if((dot_ > 0) == dir) {
            int index = atomic_inc(v_sizes + S0_SIZE_OFFSET);
//            int new_id = atomic_inc(v_sizes + DATA_SIZE_OFFSET);

            v_s0[index] = this_id;
//            v_data[new_id].orig_id = s2.orig_id;
            v_data[this_id].strength = s2.strength * fabs(dot_);
            v_data[this_id].start_p = s2.intersect_p;
            v_data[this_id].in_dir = p;
        }
//    }
    v_seed[global_id] = rand_seed;
}
