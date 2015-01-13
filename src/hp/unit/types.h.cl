/* 
* @Author: BlahGeek
* @Date:   2015-01-09
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-09
*/

typedef struct {
    int orig_id;
    float3 strength;
    float3 start_p, in_dir;
    int4 geometry;
    float intersect_number;
    float3 intersect_p, normal;
    float optical_density;
//
//    float3 new_strength_refract;
//    float3 new_strength_specular;
//    float3 new_strength_diffuse;
//    float3 new_strength_light;
} unit_data;

typedef struct {
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float optical_density;
    float dissolve;

    float specular_possibility;
    float refract_possibility;
    float diffuse_possibility;
} Material;
