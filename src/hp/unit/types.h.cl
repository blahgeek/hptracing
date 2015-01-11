/* 
* @Author: BlahGeek
* @Date:   2015-01-09
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-09
*/

typedef struct {
    int orig_id;
//    int depth;
    float3 strength;
    float3 start_p, in_dir;
} unit_S0;

typedef struct {
    int orig_id;
//    int depth;
    float3 strength;
    int4 geometry; // xyz is index of points, w is mat_id
    float3 start_p, in_dir;
    float intersect_number;
} unit_S1;

typedef struct {
    int orig_id;
//    int depth;
    float3 new_strength;
    float3 in_dir, normal, intersect_p;
    float optical_density;
} unit_S2_refract;

typedef struct {
    int orig_id;
//    int depth;
    float3 new_strength;
    float3 in_dir, normal, intersect_p;
} unit_S2_specular;

typedef struct {
    int orig_id;
//    int depth;
    float3 new_strength;
    float3 in_dir, normal, intersect_p;
} unit_S2_diffuse;

typedef struct {
    int orig_id;
//    int depth;
    float3 new_strength;
    float3 in_dir, normal, intersect_p;
} unit_S2_light;

typedef struct {
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float optical_density;
    float dissolve;
} Material;
