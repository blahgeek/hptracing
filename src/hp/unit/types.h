/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-13
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
    cl_float3 strength;
    cl_float3 start_p, in_dir;
    cl_int4 geometry;
    cl_float intersect_number;
    cl_float3 intersect_p, normal;
    cl_float optical_density;

    // cl_float3 new_strength_refract;
    // cl_float3 new_strength_specular;
    // cl_float3 new_strength_diffuse;
    // cl_float3 new_strength_light;
} unit_data;

typedef struct {
    cl_float3 ambient;
    cl_float3 diffuse;
    cl_float3 specular;
    cl_float optical_density;
    cl_float dissolve;

    cl_float specular_possibility;
    cl_float refract_possibility;
    cl_float diffuse_possibility;
} Material;

}
}


#endif
