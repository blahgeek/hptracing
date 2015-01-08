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
    class S2_refract; class S2_specular; class S2_diffuse;

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
                 std::vector<S2_diffuse> & s2_diffuse);
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
        int specular_exp = 1;

        int samples = 1;

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

}

class TraceUnit {
public:
    /**
     * The ID of primary light
     */
    int orig_id = -1;
    /**
     * Trace depth. Start with 0 (primary light)
     */
    int depth = 0;

    Color strength;

    Vec start_p;
    Vec in_dir;

    Geometry * geometry = nullptr;
    Material * material = nullptr;

    Vec normal;
    Vec intersect_p;

    Vec reflection_dir;
    Vec refraction_dir;

    Color result = Vec(0, 0, 0);

public:
    /**
     * Given a scene, find THE geometry which intersect with (start_p->in_dir)
     * geometry, material, intersect_p and result will be computed 
     * (geometry may remains nullptr)
     * @return True if found
     */
    bool findGeometry(Scene * scene);
    /**
     * Compute intersection information base on geometry previously found
     * normal, intersect_p, reflection_dir, refeaction_dir will be computed
     */
    void computeIntersection();

    using unit_insert_f = std::function<void(TraceUnit)>;
    /**
     * Sample all sub-trace, put it into target
     */
    void sampleSubTrace(Scene * scene, unit_insert_f insert_f);

};

}

#endif
