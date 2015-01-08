/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#include <iostream>
#include <algorithm>
#include "./base.h"
#include "../geometry/triangle.h"

using namespace hp;

Scene::Scene(int num_geo, int num_mat) {
    geometries.resize(num_geo);
    materials.resize(num_mat);
}

void Scene::setMaterial(int n, Material mat) {
    hp_assert(static_cast<size_t>(n) < materials.size());
    materials[n] = mat;
}

void Scene::setGeometry(int n, std::unique_ptr<Geometry> && geo, int mat_id) {
    geometries[n] = std::make_pair(std::move(geo), mat_id);
}

std::tuple<Number, Geometry *, Material *> Scene::intersect(const Vec & start_p, const Vec & dir) {
    Number min_num = -1;
    std::tuple<Number, Geometry *, Material *> ret = {-1, nullptr, nullptr};
    for(auto & x: geometries) {
        auto result = x.first->intersect(start_p, dir);
        if(result >= 0 && (min_num < 0 || result < min_num)) {
            min_num = result;
            ret = {result, x.first.get(), &(materials[x.second])};
        }
    }
    return ret;
}

Scene::Scene(std::string filename) {
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mats;

    auto err = tinyobj::LoadObj(shapes, mats, filename.c_str());

    if(!err.empty())
        throw err;

    hp_log("Scene loaded: %lu shapes, %lu materials", 
           shapes.size(), mats.size());

    for(auto & mat: mats) {
        this->materials.push_back({
            .ambient = Color(mat.ambient[0], mat.ambient[1], mat.ambient[2]),
            .diffuse = Color(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]),
            .specular = Color(mat.specular[0], mat.specular[1], mat.specular[2]),
            .specular_exp = int(mat.shininess),
            .optical_density = mat.ior,
            .dissolve = mat.dissolve
        });
        hp_log("Mat diffuse: %f %f %f", mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
    }

    for(auto & shape: shapes) {
        hp_log("Shape %s, indices: %ld, mats: %ld",
               shape.name.c_str(), shape.mesh.indices.size(), shape.mesh.material_ids.size());
        std::vector<Vec> vertices;
        for(size_t i = 0 ; i < shape.mesh.positions.size() ; i += 3)
            vertices.emplace_back(shape.mesh.positions[i],
                                  shape.mesh.positions[i+1],
                                  shape.mesh.positions[i+2]);
        for(size_t i = 0 ; i < shape.mesh.indices.size() ; i += 3)
            this->geometries.emplace_back(std::make_unique<Triangle>(vertices[shape.mesh.indices[i]],
                                                                     vertices[shape.mesh.indices[i+1]],
                                                                     vertices[shape.mesh.indices[i+2]]),
                                          shape.mesh.material_ids[i / 3]);
    }
}
