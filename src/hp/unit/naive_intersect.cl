
float _single_intersect(float3 start_p, float3 in_dir,
                        float3 pa, float3 pb, float3 pc) {
    float3 a = in_dir;
    float3 b = pa - pb;
    float3 c = pa - pc;
    float3 t = pa - start_p;

    float x, m, n;

    float det_a = a.x * b.y * c.z + a.y * b.z * c.x +
                     a.z * b.x * c.y - a.z * b.y * c.x -
                     a.x * b.z * c.y - c.z * a.y * b.x;
    if(det_a == 0) return -1.0;

    x = (b.y * c.z - b.z * c.y) * t.x + 
        (a.z * c.y - a.y * c.z) * t.y +
        (a.y * b.z - a.z * b.y) * t.z;
    m = (b.z * c.x - b.x * c.z) * t.x + 
        (a.x * c.z - a.z * c.x) * t.y +
        (a.z * b.x - a.x * b.z) * t.z;
    n = (b.x * c.y - b.y * c.x) * t.x +
        (a.y * c.x - a.x * c.y) * t.y + 
        (a.x * b.y - a.y * b.x) * t.z;

    if(m >= 0 && m <= 1 && n >= 0 && n <= 1 &&
       m + n < 1 && x > 0) return x;

    return -1.0;

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
    float intersect_number = -1;
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


//    if(geo_id != -1) {
        int index = atomic_inc(v_s1_size);
        v_s1[index].orig_id = s0.orig_id;
        v_s1[index].depth = s0.depth;
        v_s1[index].strength = s0.strength;
        v_s1[index].geo_id = geo_id;
        v_s1[index].mat_id = mat_id;
        v_s1[index].start_p = s0.start_p;
        v_s1[index].in_dir = s0.in_dir;
        v_s1[index].intersect_number = intersect_number;
//    }
}
