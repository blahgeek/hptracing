/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-09
*/

#include "./trace_unit.h"
#include "./scene/base.h"
#include <iostream>

using namespace hp;

#define MIN_STRENGTH 1e-3

#define DIFFICULTY 1

#define DIFFUSE_SAMPLE (16 * DIFFICULTY)
#define LIGHT_SAMPLE (2 * DIFFICULTY)

#define GENERAL_THRESHOLD (5e-3 / float(DIFFICULTY))

#define LIGHT_SAMPLE_THRESHOLD (1.0 / DIFFUSE_SAMPLE / 2.0)
#define DIFFUSE_SAMPLE_THRESHOLD (1.0 / DIFFUSE_SAMPLE * 1.5)

void Unit::S0::run(Scene * scene, std::vector<Unit::S1> & s1) {
    auto result = scene->intersect(start_p, in_dir);
    auto geo = std::get<1>(result);

    if(geo) {
        Unit::S1 x;
        x.orig_id = orig_id,
        x.depth = depth,
        x.strength = strength,
        x.geometry = geo,
        x.material = std::get<2>(result),
        x.start_p = start_p,
        x.in_dir = in_dir,
        x.intersect_number = std::get<0>(result);
        s1.push_back(x);
    }
}

void Unit::S1::run(std::vector<Color> & results,
                   std::vector<Unit::S2_refract> & s2_refract,
                   std::vector<Unit::S2_specular> & s2_specular,
                   std::vector<Unit::S2_diffuse> & s2_diffuse,
                   std::vector<Unit::S2_light> & s2_light) {
    Vec intersect_p = start_p + intersect_number * in_dir;
    Vec normal = geometry->getNormal(intersect_p);

    Color result = strength.cwiseProduct(material->ambient);
    result *= std::abs(normal.dot(in_dir));
    results[orig_id] += result;

    Color new_strength = strength * (1 - material->dissolve);
    if(new_strength.norm() > MIN_STRENGTH) {
        Unit::S2_refract x;
        x.orig_id = orig_id,
        x.depth = depth,
        x.new_strength = new_strength,
        x.in_dir = in_dir,
        x.normal = normal,
        x.intersect_p = intersect_p,
        x.optical_density = material->optical_density;
        s2_refract.push_back(x);
    }

    new_strength = strength.cwiseProduct(material->specular);
    auto new_strength_norm = new_strength.norm();
    if(new_strength_norm > GENERAL_THRESHOLD) {
        Unit::S2_specular x;
        x.orig_id = orig_id,
        x.depth = depth,
        x.new_strength = new_strength,
        x.in_dir = in_dir,
        x.normal = normal,
        x.intersect_p = intersect_p;
        s2_specular.push_back(x);
    }

    new_strength = strength.cwiseProduct(material->diffuse);
    new_strength_norm = new_strength.norm();
    if(new_strength_norm > DIFFUSE_SAMPLE_THRESHOLD) {
        Unit::S2_diffuse x;
        x.orig_id = orig_id,
        x.depth = depth,
        x.new_strength = new_strength,
        x.in_dir = in_dir,
        x.normal = normal,
        x.intersect_p = intersect_p;
        s2_diffuse.push_back(x);
    }

    // reuse diffuse strength
    new_strength /= (LIGHT_SAMPLE * DIFFUSE_SAMPLE);
    if(new_strength_norm > LIGHT_SAMPLE_THRESHOLD) {
        Unit::S2_light x;
        x.orig_id = orig_id,
        x.depth = depth,
        x.new_strength = new_strength,
        x.in_dir = in_dir,
        x.normal = normal,
        x.intersect_p = intersect_p;
        s2_light.push_back(x);
    }

}

void Unit::S2_light::run(Scene * scene, std::vector<S0> & s0) {
    bool dir = in_dir.dot(normal) < 0;
    for(int i = 0 ; i < LIGHT_SAMPLE ; i += 1) {
        Vec new_dir = scene->randomRayToLight(intersect_p);
        Number dot = new_dir.dot(normal);
        if((dot > 0) == dir) {
            Unit::S0 x;
            x.orig_id = orig_id,
            x.depth = depth + 1,
            x.strength = new_strength * std::abs(dot),
            x.start_p = intersect_p,
            x.in_dir = new_dir;
            s0.push_back(x);
        }
    }
}

void Unit::S2_refract::run(std::vector<Unit::S0> & s0) {
    Vec refraction_dir = Geometry::getRefraction(in_dir, normal, optical_density);
    Unit::S0 x;
    x.orig_id = orig_id,
    x.depth = depth + 1,
    x.strength = new_strength,
    x.start_p = intersect_p,
    x.in_dir = refraction_dir;
    s0.push_back(x);
}

void Unit::S2_specular::run(std::vector<Unit::S0> & s0) {
    Vec reflection_dir = Geometry::getReflection(in_dir, normal);

    Unit::S0 x;
    x.orig_id = orig_id,
    x.depth = depth + 1,
    x.strength = new_strength,
    x.start_p = intersect_p,
    x.in_dir = reflection_dir;
    s0.push_back(x);
}

void Unit::S2_diffuse::run(std::vector<Unit::S0> & s0) {
    for(int i = 0 ; i < DIFFUSE_SAMPLE ; i += 1) {
        Vec p(RAND_F(), RAND_F(), RAND_F()); p.normalize();
        Number dot_normal = p.dot(normal);

        if(dot_normal < 0) { // FIXME: do not use branch
            dot_normal = -dot_normal;
            if(in_dir.dot(normal) < 0)
                p = -p;
        }

        Color strength = new_strength * dot_normal / DIFFUSE_SAMPLE;
        Unit::S0 x;
        x.orig_id = orig_id,
        x.depth = depth + 1,
        x.strength = strength,
        x.start_p = intersect_p,
        x.in_dir = p;
        s0.push_back(x);
    }
}

