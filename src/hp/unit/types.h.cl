/* 
* @Author: BlahGeek
* @Date:   2015-01-09
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-09
*/

typedef struct {
    int orig_id;
    int depth;
    float3 strength;
    float3 start_p, in_dir;
} unit_S0;

typedef struct {
    int orig_id;
    int depth;
    float3 strength;
    int geo_id;
    int mat_id;
    float3 start_p, in_dir;
    float intersect_number;
} unit_S1;

