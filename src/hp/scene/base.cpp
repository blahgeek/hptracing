/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-12-29
*/

#include <iostream>
#include "./base.h"
#include "../common.h"
#include <vector>
#include <string>

#define cimg_display 0
#include "../../CImg/CImg.h"

#include "../../objloader/tiny_obj_loader.h"

#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
        (X).s[3] = 0; \
    } while(0)

#define NORM(X) ((Vec((X)[0], (X)[1], (X)[2])).norm())

#define CL_FLOAT3_TO_VEC(X) \
    hp::Vec(X.s[0], X.s[1], X.s[2])

using namespace hp;

int Scene::texture_width = 512;
int Scene::texture_height = 512;

Scene::Scene(std::string filename, std::string mtl_basepath) {
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mats;

    auto err = tinyobj::LoadObj(shapes, mats, filename.c_str(), 
                                mtl_basepath.length() == 0 ? NULL : mtl_basepath.c_str());

    if(!err.empty())
        throw err;

    for(auto & mat: mats) {
        Material x;
        ASSIGN_F3(x.emission, mat.emission);
        ASSIGN_F3(x.ambient, mat.ambient);
        ASSIGN_F3(x.diffuse, mat.diffuse);
        ASSIGN_F3(x.specular, mat.specular);
        x.optical_density = mat.ior;
        x.dissolve = mat.dissolve;
        x.shininess = mat.shininess;
        x.diffuse_texture_id = -1;
        x.ambient_texture_id = -1;

        hp_log("Reading material %s", mat.name.c_str());

        if(mat.diffuse_texname.length() > 0) {
            auto found = std::find(texture_names.begin(),
                                   texture_names.end(),
                                   mat.diffuse_texname);
            if(found != texture_names.end())
                x.diffuse_texture_id = found - texture_names.begin();
            else {
                x.diffuse_texture_id = texture_names.size();
                texture_names.push_back(mat.diffuse_texname);
            }
        }

        if(mat.ambient_texname.length() > 0) {
            auto found = std::find(texture_names.begin(),
                                   texture_names.end(),
                                   mat.ambient_texname);
            if(found != texture_names.end())
                x.ambient_texture_id = found - texture_names.begin();
            else {
                x.ambient_texture_id = texture_names.size();
                texture_names.push_back(mat.ambient_texname);
            }
        }

    #define DIFFUSE_FACTOR 0.9f // relative to (diffuse + light)

        Number specular_length = NORM(mat.specular);
        Number diffuse_length = NORM(mat.diffuse);
        Number refract_length = 1.0f - mat.dissolve;
        Number sum = specular_length + diffuse_length + refract_length + 1e-4;
        x.specular_possibility = specular_length / sum;
        x.refract_possibility = refract_length / sum + x.specular_possibility;
        x.diffuse_possibility = diffuse_length * DIFFUSE_FACTOR / sum + x.refract_possibility;
        // x.diffuse_possibility = x.refract_possibility;
        this->materials.push_back(x);
    }

    for(auto & texture_name: texture_names) {
        std::string texture_path = mtl_basepath + texture_name;
        hp_log("Reading texture %s", texture_path.c_str());
        auto image = cimg_library::CImg<>(texture_path.c_str())
                     .normalize(0, 255).resize(texture_width, texture_height, 1, 3);
        for(int i = 0 ; i < texture_height ; i += 1) {
            for(int j = 0 ; j < texture_width ; j += 1) {
                for(int k = 0 ; k < 3 ; k += 1)
                    this->texture_data.push_back(image(j, i, k));
                this->texture_data.push_back(0);
            }
        }
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
        if(shape.mesh.normals.size() == 0) 
            hp_log("WARNING: No normal data found");
        for(size_t i = 0 ; i < shape.mesh.normals.size() ; i += 3) {
            cl_float3 normal;
            normal.s[0] = shape.mesh.normals[i];
            normal.s[1] = shape.mesh.normals[i+1];
            normal.s[2] = shape.mesh.normals[i+2];
            this->normals.push_back(normal);
        }
        if(shape.mesh.texcoords.size() == 0)
            hp_log("WARNING: No texcoord data found");
        for(size_t i = 0 ; i < shape.mesh.texcoords.size() ; i += 2) {
            cl_float2 coord;
            coord.s[0] = shape.mesh.texcoords[i];
            coord.s[1] = shape.mesh.texcoords[i+1];
            this->texcoords.push_back(coord);
        }
        for(size_t i = 0 ; i < shape.mesh.indices.size() ; i += 3) {
            cl_int4 triangle;
            triangle.s[0] = shape.mesh.indices[i] + point_base_index;
            triangle.s[1] = shape.mesh.indices[i+1] + point_base_index;
            triangle.s[2] = shape.mesh.indices[i+2] + point_base_index;
            triangle.s[3] = shape.mesh.material_ids[i / 3];
            this->geometries.push_back(triangle);
            this->registerGeometry(triangle);

            // hp_log("   Triangle: %d %d %d %d", triangle.s[0], triangle.s[1], triangle.s[2], triangle.s[3]);
        }
    }

    this->finishRegister();
    
    hp_log("Scene loaded: %lu shapes, %lu materials", 
           shapes.size(), mats.size());

}

void Scene::registerGeometry(cl_int4 triangle) {
    Vec pa = CL_FLOAT3_TO_VEC(points[triangle.s[0]]);
    Vec pb = CL_FLOAT3_TO_VEC(points[triangle.s[1]]);
    Vec pc = CL_FLOAT3_TO_VEC(points[triangle.s[2]]);
    Material mat = materials[triangle.s[3]];
    Number emission_norm = CL_FLOAT3_TO_VEC(mat.emission).norm();
    if(emission_norm > 0) {
        Vec dx = pb - pa;
        Vec dy = pc - pa;
        Number dot = dx.dot(dy);
        Number cosine = dx.normalized().dot(dy.normalized());
        Number area = dot / cosine * std::sqrt(1 - cosine * cosine);

        Number val = area * emission_norm;
        total_light_val += val;
        lights_map[total_light_val] = geometries.size() - 1;
    }
}

#define LIGHTS_TOTAL_NUMBER 256

void Scene::finishRegister() {
    if(lights_map.size() == 0) {
        hp_log("WARNING: No emission material in scene!!!");
        lights.clear();
        return;
    }
    // hp_assert(lights_map.size() > 0);
    for(int i = 0 ; i < LIGHTS_TOTAL_NUMBER ; i += 1) {
        Number val = Number(i) / Number(LIGHTS_TOTAL_NUMBER) * total_light_val;
        auto target = lights_map.upper_bound(val);
        if(target == lights_map.end()) target = lights_map.begin();

        lights.push_back(geometries[target->second]);
    }

    total_light_val = 0;
    lights_map.clear();
}
