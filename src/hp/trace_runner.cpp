/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#include "./trace_runner.h"
#include "./scene/base.h"
#include <iostream>

using namespace hp;

void TraceRunner::run() {
    units.clear();
    state0.clear(); state1.clear(); state2.clear();
    // put every view_dir into units0
    TraceUnit unit;
    unit.orig_id = -1;
    unit.strength = {1, 1, 1};
    unit.start_p = view_p;
    for(auto & dir: view_dir) {
        unit.orig_id += 1;
        unit.in_dir = dir;
        units.push_back(unit);
        state0.insert(unit.orig_id);
    }

    while(!(state0.empty() && state1.empty() && state2.empty())) {
        std::cerr << "Loop: state0 " << state0.size() 
            << ", state1 " << state1.size() 
            << ", state2 " << state2.size() << std::endl;
            
        for(auto index: state0) {
            TraceUnit & unit = units[index];
            if(unit.findGeometry(scene.get()))
                state1.insert(index);
        }
        state0.clear();

        std::cerr << "Loop1: state0 " << state0.size() 
            << ", state1 " << state1.size() 
            << ", state2 " << state2.size() << std::endl;
 
        for(auto index: state1) {
            TraceUnit & unit = units[index];
            unit.computeIntersection();
            state2.insert(index);
        }
        state1.clear();

        std::cerr << "Loop2: state0 " << state0.size() 
            << ", state1 " << state1.size() 
            << ", state2 " << state2.size() << std::endl;
 

        for(auto index: state2) {
            TraceUnit & unit = units[index];
            unit.sampleSubTrace(scene.get(), [this](TraceUnit x) {
                state0.insert(units.size());
                units.push_back(x);
            });
        }
        state2.clear();
    }

    result.resize(view_dir.size(), Color(0, 0, 0));

    for(auto & unit: units)
        result[unit.orig_id] += unit.result;

    units.clear();
}
