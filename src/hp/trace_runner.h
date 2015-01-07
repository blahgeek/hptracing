/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
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
    std::vector<TraceUnit> units;
    std::set<int> state0, state1, state2;

    std::vector<Vec> view_dir;
    Vec view_p;

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
