/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#ifndef __hp_trace_unit_h__
#define __hp_trace_unit_h__ value

#include <vector>
#include <functional>
#include "./common.h"
#include "./material.h"
#include "./geometry/base.h"

using hp::Vec;
using hp::Color;
using hp::Material;
using hp::Geometry;

namespace hp {

class Scene;

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
