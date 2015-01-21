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
    float shininess;
} unit_data;

typedef struct {
    float3 emission;
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float optical_density;
    float dissolve;
    float shininess;

    int diffuse_texture_id;
    int ambient_texture_id;

    float specular_possibility;
    float refract_possibility;
    float diffuse_possibility;
} Material;

typedef struct {
    float3 box_start;
    float3 box_end;
    int child; // first child, -1 for null
    int parent;
    int sibling;
    int data;
} KDTreeNodeHeader;
