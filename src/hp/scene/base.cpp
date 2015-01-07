/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#include <iostream>
#include <algorithm>
#include "./base.h"

using namespace hp;

Scene::Scene(int num_geo, int num_mat) {
    geometries.resize(num_geo);
    materials.resize(num_mat);
}

void Scene::setMaterial(int n, Material mat) {
    hp_assert(n < materials.size());
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
