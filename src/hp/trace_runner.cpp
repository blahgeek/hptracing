/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-10
*/

#include "./trace_runner.h"
#include "./scene/base.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace hp;

void TraceRunner::log(int i) {
    std::cerr << "Loop " << i
        << ": s0 " << s0.size() 
        << ", s1 " << s1.size() 
        << ", s2_refract " << s2_refract.size() 
        << ", s2_specular " << s2_specular.size() 
        << ", s2_light " << s2_light.size()
        << ", s2_diffuse " << s2_diffuse.size() << std::endl;
}

void TraceRunner::run() {
    std::srand(std::time(0));

    result.resize(view_dir.size(), Color(0, 0, 0));
    size_t orig_id = 0;

    while(orig_id < view_dir.size()) {

        s0.clear(); s1.clear(); 
        s2_specular.clear(); s2_refract.clear(); s2_diffuse.clear(); s2_light.clear();

        for(size_t i = 0 ; i < 10000 && orig_id + i < view_dir.size(); i += 1) {
            Unit::S0 x;
            x.orig_id = orig_id + i,
            x.depth = 0,
            x.strength = Color(1, 1, 1),
            x.start_p = view_p,
            x.in_dir = view_dir[orig_id + i];
            s0.push_back(x);
        }
        orig_id += 10000;

        for(int i = 0 ; i < 5 ; i += 1) {

            log(i);

            for(auto & x: s0)
                x.run(scene.get(), s1);
            s0.clear();

            log(i);

            for(auto & x: s1)
                x.run(result, s2_refract, s2_specular, s2_diffuse, s2_light);
            s1.clear();
            
            log(i);

            for(auto & x: s2_refract)
                x.run(s0);
            s2_refract.clear();
            
            log(i);

            for(auto & x: s2_diffuse)
                x.run(s0);
            s2_diffuse.clear();
            
            log(i);

            for(auto & x: s2_light)
                x.run(scene.get(), s0);
            s2_light.clear();

            log(i);

            for(auto & x: s2_specular)
                x.run(s0);
            s2_specular.clear();
            
            log(i);
        }

    }

    
}
