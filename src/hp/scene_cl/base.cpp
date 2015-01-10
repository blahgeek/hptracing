/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-10
*/

#include <iostream>
#include "./base.h"
#include "../common.h"
#include <vector>
#include <string>

#include "../../objloader/tiny_obj_loader.h"

#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
    } while(0)

#define CL_FLOAT3_TO_VEC(X) \
    hp::Vec(X.s[0], X.s[1], X.s[2])

using namespace hp;

cl::Scene::Scene(std::string filename) {
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mats;

    auto err = tinyobj::LoadObj(shapes, mats, filename.c_str());

    if(!err.empty())
        throw err;

    for(auto & mat: mats) {
        cl::Material x;
        ASSIGN_F3(x.ambient, mat.ambient);
        ASSIGN_F3(x.diffuse, mat.diffuse);
        ASSIGN_F3(x.specular, mat.specular);
        x.optical_density = mat.ior;
        x.dissolve = mat.dissolve;
        this->materials.push_back(x);
    }

    for(auto & shape: shapes) {
        hp_log("Shape %s, indices: %ld, mats: %ld",
               shape.name.c_str(), shape.mesh.indices.size(), shape.mesh.material_ids.size());
        size_t point_base_index = this->points.size();
        for(size_t i = 0 ; i < shape.mesh.positions.size() ; i += 3) {
            cl_float3 point;
            point.s[0] = shape.mesh.positions[i];
            point.s[1] = shape.mesh.positions[i+1];
            point.s[2] = shape.mesh.positions[i+2];
            this->points.push_back(point);
        }
        for(size_t i = 0 ; i < shape.mesh.indices.size() ; i += 3) {
            cl_int4 triangle;
            triangle.s[0] = shape.mesh.indices[i] + point_base_index;
            triangle.s[1] = shape.mesh.indices[i+1] + point_base_index;
            triangle.s[2] = shape.mesh.indices[i+2] + point_base_index;
            triangle.s[3] = shape.mesh.material_ids[i / 3];
            this->geometries.push_back(triangle);
            this->registerGeometry(triangle);

            hp_log("   Triangle: %d %d %d %d", triangle.s[0], triangle.s[1], triangle.s[2], triangle.s[3]);
        }
    }

    this->finishRegister();
    
    hp_log("Scene loaded: %lu shapes, %lu materials", 
           shapes.size(), mats.size());

}

void cl::Scene::registerGeometry(cl_int4 triangle) {
    Vec pa = CL_FLOAT3_TO_VEC(points[triangle.s[0]]);
    Vec pb = CL_FLOAT3_TO_VEC(points[triangle.s[1]]);
    Vec pc = CL_FLOAT3_TO_VEC(points[triangle.s[2]]);
    cl::Material mat = materials[triangle.s[3]];
    Number ambient_norm = CL_FLOAT3_TO_VEC(mat.ambient).norm();
    if(ambient_norm > 0) {
        Vec dx = pb - pa;
        Vec dy = pc - pa;
        Number dot = dx.dot(dy);
        Number cosine = dx.normalized().dot(dy.normalized());
        Number area = dot / cosine * std::sqrt(1 - cosine * cosine);

        Number val = area * ambient_norm;
        total_light_val += val;
        lights_map[total_light_val] = geometries.size() - 1;
    }
}

#define LIGHTS_TOTAL_NUMBER 256

void cl::Scene::finishRegister() {
    hp_assert(lights_map.size() > 0);
    for(int i = 0 ; i < LIGHTS_TOTAL_NUMBER ; i += 1) {
        Number val = Number(i) / Number(LIGHTS_TOTAL_NUMBER) * total_light_val;
        auto target = lights_map.upper_bound(val);
        if(target == lights_map.end()) target = lights_map.begin();

        lights.push_back(geometries[target->second]);
    }

    total_light_val = 0;
    lights_map.clear();
}
