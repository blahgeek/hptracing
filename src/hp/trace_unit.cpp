/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#include "./trace_unit.h"
#include "./scene/base.h"

using namespace hp;

#define MIN_STRENGTH 1e-3
#define REFLECT_SAMPLE 64

bool TraceUnit::findGeometry(Scene * scene) {
    auto result = scene->intersect(start_p, in_dir);
    this->geometry = std::get<1>(result);
    if(this->geometry) { // split this to another kernel?
        this->material = std::get<2>(result);
        this->intersect_p = start_p + std::get<0>(result) * in_dir;
        this->result = strength.cwiseProduct(material->ambient);
        return true;
    }
    return false;
}

void TraceUnit::computeIntersection() {
    if(!geometry) return; // should be compacted when using OpenCL
    this->normal = geometry->getNormal(intersect_p);
    this->reflection_dir = Geometry::getReflection(in_dir, normal);
    this->refraction_dir = Geometry::getRefraction(in_dir, normal, 
                                                   material->optical_density);
}

void TraceUnit::sampleSubTrace(Scene * scene, TraceUnit::unit_insert_f insert_f) {
    if(!geometry) return;
    // refract
    Color new_strength = strength * (1 - material->dissolve);
    if(new_strength.norm() > MIN_STRENGTH) { // compact?
        TraceUnit unit;
        unit.orig_id = this->orig_id;
        unit.depth = this->depth + 1;
        unit.strength = new_strength;
        unit.start_p = this->intersect_p;
        unit.in_dir = this->refraction_dir;
        insert_f(unit);
    }
    // reflect
    for(int i = 0 ; i < REFLECT_SAMPLE ; i += 1) {
        Number rand_alpha = Number(rand()) / Number(RAND_MAX) * 2 * PI;
        Number rand_beta = Number(rand()) / Number(RAND_MAX) * 2 * PI;
        Vec p(std::cos(rand_alpha) * std::cos(rand_beta), 
              std::sin(rand_alpha) * std::cos(rand_beta), 
              std::sin(rand_beta));
        Number dot_normal = p.dot(normal);
        if(dot_normal < 0){
            dot_normal = -dot_normal;
            p = -p;
        }

        Color specular_term = material->specular * 
                std::pow(p.dot(reflection_dir), material->specular_exp);
        Color diffuse_term = material->diffuse * dot_normal;

        new_strength = strength.cwiseProduct(specular_term + diffuse_term) / Number(REFLECT_SAMPLE);
        if(new_strength.norm() > MIN_STRENGTH) {
            TraceUnit unit;
            unit.orig_id = this->orig_id;
            unit.depth = this->depth + 1;
            unit.strength = new_strength;
            unit.start_p = this->intersect_p;
            unit.in_dir = p;
            insert_f(unit);
        }
    }
}
