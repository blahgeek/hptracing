/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#ifndef __hp_trace_unit_h__
#define __hp_trace_unit_h__ value

#include <vector>
#include <functional>
#include "./common.h"
#include "./material.h"
#include "./geometry/base.h"
#include "./scene/base.h"

using hp::Vec;
using hp::Color;
using hp::Material;
using hp::Geometry;

namespace hp {

namespace Unit {

    class S0; class S1; 
    class S2_refract; class S2_specular; class S2_diffuse; class S2_light;

    class S0 {
    public:
        int orig_id = -1;
        int depth = 0;
        Color strength = {0, 0, 0};
        Vec start_p, in_dir;

        void run(Scene * scene, std::vector<S1> & s1);
    };

    class S1 {
    public:
        int orig_id = -1;
        int depth = 0;
        Color strength;
        Geometry * geometry = nullptr;
        Material * material = nullptr;
        Vec start_p, in_dir;
        Number intersect_number;

        void run(std::vector<Color> & results,
                 std::vector<S2_refract> & s2_refract,
                 std::vector<S2_specular> & s2_specular,
                 std::vector<S2_diffuse> & s2_diffuse,
                 std::vector<S2_light> & s2_light);
    };

    class S2_refract {
    public:
        int orig_id = -1;
        int depth = 0;
        Color new_strength;
        Vec in_dir, normal, intersect_p;
        Number optical_density = 1;

        void run(std::vector<S0> & s0);
    };

    class S2_specular {
    public:
        int orig_id = -1;
        int depth = 0;
        Color new_strength;
        Vec in_dir, normal, intersect_p;
        // int specular_exp = 1;

        // int samples = 1;

        void run(std::vector<S0> & s0);
    };

    class S2_diffuse {
    public:
        int orig_id = -1;
        int depth = 0;
        Color new_strength;
        Vec in_dir, normal, intersect_p;

        // int samples = 1;

        void run(std::vector<S0> & s0);
    };

    class S2_light {
    public:
        int orig_id = -1;
        int depth = 0;
        Color new_strength;
        Vec in_dir, normal, intersect_p;

        void run(Scene * scene, std::vector<S0> & s0);
    };

}

}

#endif
