/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#ifndef __hp_trace_runner_h__
#define __hp_trace_runner_h__ value

#include "./trace_unit.h"
#include <set>

using hp::TraceUnit;

namespace hp {

class TraceRunner {

protected:
    std::unique_ptr<Scene> scene;

    std::vector<Unit::S0> s0;
    std::vector<Unit::S1> s1;
    std::vector<Unit::S2_specular> s2_specular;
    std::vector<Unit::S2_diffuse> s2_diffuse;
    std::vector<Unit::S2_refract> s2_refract;
    // std::vector<TraceUnit> units;
    // std::set<int> state0, state1, state2;

    std::vector<Vec> view_dir;
    Vec view_p;

    void log();

public:
    TraceRunner(std::unique_ptr<Scene> && scene, 
                std::vector<Vec> && view_dir,
                Vec view_p): 
    scene(std::move(scene)), view_dir(view_dir), view_p(view_p) {}

    std::vector<Color> result;

    void run();

};

}

#endif
