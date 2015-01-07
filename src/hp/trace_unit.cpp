/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#include "./trace_unit.h"
#include "./scene/base.h"
#include <iostream>

using namespace hp;

#define MIN_STRENGTH 1e-4
#define SPECULAR_SAMPLE 6
#define DIFFUSE_SAMPLE 200

#define RAND_F() (Number(std::rand()) / Number(RAND_MAX) - 0.5)

bool TraceUnit::findGeometry(Scene * scene) {

    auto result = scene->intersect(start_p, in_dir);
    this->geometry = std::get<1>(result);
    if(this->geometry) { // split this to another kernel?
        this->material = std::get<2>(result);
        this->intersect_p = start_p + std::get<0>(result) * in_dir;
        this->normal = geometry->getNormal(intersect_p);
        this->result = strength.cwiseProduct(material->ambient);
        this->result *= std::abs(normal.dot(in_dir));
        return true;
    }
    return false;
}

void TraceUnit::computeIntersection() {
    this->reflection_dir = Geometry::getReflection(in_dir, normal);
    this->refraction_dir = Geometry::getRefraction(in_dir, normal, 
                                                   material->optical_density);
}

void TraceUnit::sampleSubTrace(Scene * scene, TraceUnit::unit_insert_f insert_f) {

    std::vector<TraceUnit> tmp_unit;
    // refract
    Color new_strength = strength * (1 - material->dissolve);
    if(new_strength.norm() > MIN_STRENGTH) { // compact?
        TraceUnit unit;
        unit.orig_id = this->orig_id;
        unit.depth = this->depth + 1;
        unit.strength = new_strength;
        unit.start_p = this->intersect_p;
        unit.in_dir = this->refraction_dir;
        tmp_unit.push_back(unit);
    }
    // specular reflect
    new_strength = strength.cwiseProduct(material->specular);
    if(new_strength.norm() > MIN_STRENGTH) {
        int samples = int(std::sqrt(std::max(1, 100 - material->specular_exp)));
        TraceUnit unit;
        unit.orig_id = this->orig_id;
        unit.depth = this->depth + 1;
        unit.strength = new_strength / samples;
        unit.start_p = this->intersect_p;
        for(int i = 0 ; i < samples ; i += 1) {
            Vec new_dir = this->reflection_dir;
            new_dir[0] += std::pow(RAND_F(), material->specular_exp);
            new_dir[1] += std::pow(RAND_F(), material->specular_exp);
            new_dir[2] += std::pow(RAND_F(), material->specular_exp);
            new_dir.normalize();
            unit.in_dir = new_dir;
            tmp_unit.push_back(unit);
        }
    }
    // diffuse
    for(int i = 0 ; i < DIFFUSE_SAMPLE ; i += 1) {
        Vec p(RAND_F(), RAND_F(), RAND_F()); p.normalize();
        Number dot_normal = p.dot(normal);

        if(dot_normal < 0 && reflection_dir.dot(normal) > 0){
            dot_normal = -dot_normal;
            p = -p;
        }

        // Color specular_term(0, 0, 0);
        // Number specular_dot = p.dot(reflection_dir);
        // if(specular_dot > 0)
        //     specular_term = material->specular * std::pow(specular_dot, material->specular_exp);
        // Color diffuse_term = material->diffuse * dot_normal;

        // // hp_assert(p.dot(normal) > 0);

        new_strength = strength.cwiseProduct(material->diffuse * dot_normal) / DIFFUSE_SAMPLE;
        if(new_strength.norm() > MIN_STRENGTH) {
            TraceUnit unit;
            unit.orig_id = this->orig_id;
            unit.depth = this->depth + 1;
            unit.strength = new_strength;
            unit.start_p = this->intersect_p;
            unit.in_dir = p;
            tmp_unit.push_back(unit);
        }
    }


    for(auto & unit: tmp_unit)
        insert_f(unit);
}
