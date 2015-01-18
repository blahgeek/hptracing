/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-18
*/

#ifndef __hp_trace_runner_cl_h__
#define __hp_trace_runner_cl_h__ value

#include "./cl_program.h"
#include "./scene/kdtree.h"

#define __CL_ENABLE_EXCEPTIONS
#include "./cl.hpp"
#include <vector>
#include <memory>

namespace hp {

    class TraceRunner {
    protected:
        cl::Context context;
        std::vector<cl::Device> devices;
        cl::Program program;

        std::unique_ptr<hp::KDTree> scene;
        std::vector<cl_float3> view_dir;
        cl_float3 view_p;

        std::vector<unit_data> unit_data_all;
    public:
        TraceRunner(std::unique_ptr<hp::KDTree> && scene,
                    std::vector<cl_float3> && view_dir,
                    cl_float3 view_p);

        std::vector<cl_float> result;
        void run();
    };

}
#endif
