/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-14
*/

#ifndef __hp_trace_runner_cl_h__
#define __hp_trace_runner_cl_h__ value

#include "./unit/types.h"
#include "./cl_program.h"
#include "./scene_cl/base.h"
#include "./scene_cl/kdtree.h"
#include "./cl_program.h"
#include <vector>
#include <memory>

namespace hp {
namespace cl {

    class TraceRunner {
    protected:
        std::unique_ptr<hp::cl::KDTree> scene;
        std::unique_ptr<hp::CLProgram> cl_program;
        std::vector<cl_float3> view_dir;
        cl_float3 view_p;
    public:
        TraceRunner(std::unique_ptr<hp::cl::KDTree> && scene,
                    std::vector<cl_float3> && view_dir,
                    cl_float3 view_p) :
        scene(std::move(scene)), view_dir(std::move(view_dir)), view_p(view_p) {
            cl_program = std::make_unique<hp::CLProgram>();
        }

        std::vector<cl_float> result;
        void run();
    };

}
}
#endif
