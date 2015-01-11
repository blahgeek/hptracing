/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-11
*/

#ifndef __hp_unit_types_h__
#define __hp_unit_types_h__ value

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace hp{
namespace cl {

typedef struct {
    cl_int orig_id;
    // cl_int depth;
    cl_float3 strength;
    cl_float3 start_p, in_dir;
} unit_S0;

typedef struct {
    cl_int orig_id;
    // cl_int depth;
    cl_float3 strength;
    cl_int4 geometry; // xyz is index of points, w is mat_id
    cl_float3 start_p, in_dir;
    cl_float intersect_number;
} unit_S1;

typedef struct {
    cl_int orig_id;
    // cl_int depth;
    cl_float3 new_strength;
    cl_float3 in_dir, normal, intersect_p;
    cl_float optical_density;
} unit_S2_refract;

typedef struct {
    cl_int orig_id;
    // cl_int depth;
    cl_float3 new_strength;
    cl_float3 in_dir, normal, intersect_p;
} unit_S2_specular;

typedef struct {
    cl_int orig_id;
    // cl_int depth;
    cl_float3 new_strength;
    cl_float3 in_dir, normal, intersect_p;
} unit_S2_diffuse;

typedef struct {
    cl_int orig_id;
    // cl_int depth;
    cl_float3 new_strength;
    cl_float3 in_dir, normal, intersect_p;
} unit_S2_light;

typedef struct {
    cl_float3 ambient;
    cl_float3 diffuse;
    cl_float3 specular;
    cl_float optical_density;
    cl_float dissolve;
} Material;

}
}


#endif
