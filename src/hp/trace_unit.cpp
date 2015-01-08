/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#include "./trace_unit.h"
#include "./scene/base.h"
#include <iostream>

using namespace hp;

#define MIN_STRENGTH 1e-3

#define SPECULAR_SAMPLE 10
#define DIFFUSE_SAMPLE 1000

#define SAMPLE_THRESHOLD 0.5f

#define RAND_F() (Number(std::rand()) / Number(RAND_MAX) - 0.5)

void Unit::S0::run(Scene * scene, std::vector<Unit::S1> & s1) {
    auto result = scene->intersect(start_p, in_dir);
    auto geo = std::get<1>(result);
    if(geo) {
        s1.push_back({
            .orig_id = orig_id,
            .depth = depth,
            .strength = strength,
            .geometry = geo,
            .material = std::get<2>(result),
            .start_p = start_p,
            .in_dir = in_dir,
            .intersect_number = std::get<0>(result),
        });
    }
}

void Unit::S1::run(std::vector<Color> & results,
                   std::vector<Unit::S2_refract> & s2_refract,
                   std::vector<Unit::S2_specular> & s2_specular,
                   std::vector<Unit::S2_diffuse> & s2_diffuse) {
    Vec intersect_p = start_p + intersect_number * in_dir;
    Vec normal = geometry->getNormal(intersect_p);

    Color result = strength.cwiseProduct(material->ambient);
    result *= std::abs(normal.dot(in_dir));
    results[orig_id] += result;

    Color new_strength = strength * (1 - material->dissolve);
    if(new_strength.norm() > MIN_STRENGTH)
        s2_refract.push_back({
            .orig_id = orig_id,
            .depth = depth,
            .new_strength = new_strength,
            .in_dir = in_dir,
            .normal = normal,
            .intersect_p = intersect_p,
            .optical_density = material->optical_density
        });

    new_strength = strength.cwiseProduct(material->specular);
    auto new_strength_norm = new_strength.norm();
    if(new_strength_norm > MIN_STRENGTH)
        s2_specular.push_back({
            .orig_id = orig_id,
            .depth = depth,
            .new_strength = new_strength,
            .in_dir = in_dir,
            .normal = normal,
            .intersect_p = intersect_p,
            .specular_exp = material->specular_exp,
            .samples = (new_strength_norm > SAMPLE_THRESHOLD) ? SPECULAR_SAMPLE : 1
        });

    new_strength = strength.cwiseProduct(material->diffuse);
    new_strength_norm = new_strength.norm();
    if(new_strength_norm > SAMPLE_THRESHOLD)
        s2_diffuse.push_back({
            .orig_id = orig_id,
            .depth = depth,
            .new_strength = new_strength,
            .in_dir = in_dir,
            .normal = normal,
            .intersect_p = intersect_p,
            // .samples = (new_strength_norm > SAMPLE_THRESHOLD) ? DIFFUSE_SAMPLE : 1
        });

}

void Unit::S2_refract::run(std::vector<Unit::S0> & s0) {
    Vec refraction_dir = Geometry::getRefraction(in_dir, normal, optical_density);
    s0.push_back({
        .orig_id = orig_id,
        .depth = depth + 1,
        .strength = new_strength,
        .start_p = intersect_p,
        .in_dir = refraction_dir
    });
}

void Unit::S2_specular::run(std::vector<Unit::S0> & s0) {
    Vec reflection_dir = Geometry::getReflection(in_dir, normal);
    bool dir = (reflection_dir.dot(normal)) > 0;

    for(int i = 0 ; i < samples ; i += 1) {
        Vec new_dir = reflection_dir;
        if(i > 0) {
            new_dir[0] += std::pow(RAND_F(), specular_exp);
            new_dir[1] += std::pow(RAND_F(), specular_exp);
            new_dir[2] += std::pow(RAND_F(), specular_exp);
            if((new_dir.dot(normal)) != dir) continue;
            new_dir.normalize();
        }
        s0.push_back({
            .orig_id = orig_id,
            .depth = depth + 1,
            .strength = new_strength,
            .start_p = intersect_p,
            .in_dir = new_dir
        });
    }
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
        s0.push_back({
            .orig_id = orig_id,
            .depth = depth + 1,
            .strength = strength,
            .start_p = intersect_p,
            .in_dir = p
        });
    }
}

