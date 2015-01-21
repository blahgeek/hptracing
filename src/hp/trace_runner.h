/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-21
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
    public:
        typedef std::pair<cl::Buffer, cl_int> VectorBuf;

    private:
        template <typename T>
        VectorBuf vec2buf(std::vector<T> v, bool readonly = true);

        void _setKernelArgs(cl::Kernel & k, int n){return ; }
        template<typename T, typename... TArgs>
        void _setKernelArgs(cl::Kernel & k, int n, T arg0, TArgs... FArgs);
        template<typename... TArgs>
        void setKernelArgs(cl::Kernel & k, TArgs... FArgs){ _setKernelArgs(k, 0, FArgs...); }

    protected:
        cl::Context context;
        std::vector<cl::Device> devices;
        cl::Program program;

        VectorBuf points_vbuf, kdtree_node_vbuf, kdtree_leaf_data_vbuf,
                  geometries_vbuf, lights_vbuf, texcoords_vbuf,
                  materials_vbuf, normals_vbuf;
        #ifdef CL_VERSION_1_2
        cl::Image2DArray texture_mem;
        #else
        cl::Buffer texture_mem;
        #endif

    public:
        TraceRunner(std::unique_ptr<hp::KDTree> & scene);

        void run(unsigned char * result_data,
                 cl_float3 view_p,
                 cl_float3 top_dir,
                 cl_float3 right_dir,
                 cl_float width,
                 cl_float height,
                 int sample_x, int sample_y,
                 int supersample_x, int supersample_y,
                 cl_float3 background_color,
                 int sample = 10,
                 int depth = 6,
                 int disable_diffuse = 0,
                 cl_float brightness = 1.0);
    };

}
#endif
