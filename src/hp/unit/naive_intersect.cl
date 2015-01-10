
float _single_intersect(float3 start_p, float3 in_dir,
                        float3 pa, float3 pb, float3 pc) {
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
       && m + n < 1 && x > 0) return x;

    return -44;

}

__kernel void naive_intersect(__global unit_S0 * v_s0,
                              __global unit_S1 * v_s1,
                              __global int * v_s1_size,
                              __global float3 * scene_points,
                              __global int4 * scene_mesh,
                              const int scene_mesh_size) {
    int global_id = get_global_id(0);

    unit_S0 s0 = v_s0[global_id];

    int mat_id = -1;
    int geo_id = -1;
    float intersect_number = -42;
    for(int i = 0 ; i < scene_mesh_size ; i += 1) {
        int4 triangle = scene_mesh[i];
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
        int index = atomic_inc(v_s1_size);
        v_s1[index].orig_id = s0.orig_id;
        v_s1[index].depth = s0.depth;
        v_s1[index].strength = s0.strength;
        v_s1[index].geo_id = geo_id;
        v_s1[index].mat_id = mat_id;
        v_s1[index].start_p = s0.start_p;
        v_s1[index].in_dir = s0.in_dir;
        v_s1[index].intersect_number = intersect_number;
    }
}
