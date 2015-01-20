/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-20
*/

#include <iostream>
#include "./trace_runner.h"
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <sys/time.h>
#include <fstream>

#include "./cl_src/types.h.cl.inc"
#include "./cl_src/kernel.cl.inc"

using namespace hp;

template <typename T>
TraceRunner::VectorBuf TraceRunner::vec2buf(std::vector<T> v, bool readonly) {
    TraceRunner::VectorBuf ret;
    ret.second = v.size();

    auto flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
    if(readonly)
        flags = CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR;

    if(v.size() > 0)
        ret.first = cl::Buffer(context, flags, sizeof(T) * v.size(), v.data());
    return ret;
}

template<typename T, typename... TArgs>
void TraceRunner::_setKernelArgs(cl::Kernel & k, int n, T arg0, TArgs... FArgs) {
    k.setArg(n, arg0);
    _setKernelArgs(k, n+1, FArgs...);
}

TraceRunner::TraceRunner(std::unique_ptr<hp::KDTree> & scene) {
    context = cl::Context(CL_DEVICE_TYPE_GPU);
    devices = context.getInfo<CL_CONTEXT_DEVICES>();

    std::string sources;
    sources.resize(src_hp___cl_src_types_h_cl_len + src_hp___cl_src_kernel_cl_len);
    memcpy(static_cast<void *>(&sources.front()), 
           src_hp___cl_src_types_h_cl, src_hp___cl_src_types_h_cl_len);
    memcpy(static_cast<void *>(&sources.front() + src_hp___cl_src_types_h_cl_len),
           src_hp___cl_src_kernel_cl, src_hp___cl_src_kernel_cl_len);

    try {
        program = cl::Program(context, sources);
        program.build(devices);
    } catch(cl::Error & err) {
        auto info = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
        std::cerr << info << std::endl;
        throw err;
    }

    std::srand(std::time(0));

    auto kdtree_data = scene->getData();

    points_vbuf = vec2buf(scene->points);
    kdtree_node_vbuf = vec2buf(kdtree_data.first);
    kdtree_leaf_data_vbuf = vec2buf(kdtree_data.second);
    geometries_vbuf = vec2buf(scene->geometries);
    lights_vbuf = vec2buf(scene->lights);
    texcoords_vbuf = vec2buf(scene->texcoords);
    materials_vbuf = vec2buf(scene->materials);
    normals_vbuf = vec2buf(scene->normals);

#ifdef CL_VERSION_1_2
    if(scene->texture_names.size() > 0)
        texture_mem = cl::Image2DArray(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), 
                                       scene->texture_names.size(),
                                       Scene::texture_width,
                                       Scene::texture_height,
                                       0, 0, scene->texture_data.data());
    else texture_mem = cl::Image2DArray(context, CL_MEM_READ_ONLY,
                                        cl::ImageFormat(CL_RGBA, CL_UNORM_INT8),
                                        1, 1, 1, 0, 0);
#else
    if(scene->texture_names.size() > 0)
        texture_mem = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 4 * Scene::texture_width * Scene::texture_height * scene->texture_names.size(),
                                 scene->texture_data.data());
#endif
}

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

std::vector<cl_float> TraceRunner::run(std::vector<cl_float3> & view_dir, cl_float3 view_p, int sample, int depth) {
    cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);

    std::vector<unit_data> unit_data_all(view_dir.size());

    for(size_t i = 0 ; i < view_dir.size() ; i += 1) {
        unit_data x;
        x.orig_id = i;
        x.strength.s[0] = x.strength.s[1] = x.strength.s[2] = 1;
        x.strength.s[3] = 0;
        x.start_p = view_p;
        x.in_dir = view_dir[i];
        unit_data_all[i] = x;
    }

    // Result data
    cl::Buffer results_mem(context, CL_MEM_READ_WRITE,
                           sizeof(cl_float) * view_dir.size() * 3);

    // Units...
    cl::Buffer sizes_mem(context, CL_MEM_READ_WRITE,
                         sizeof(cl_int) * 10);

    std::vector<cl_int> initial_s0(view_dir.size());
    for(size_t i = 0 ; i < view_dir.size() ; i += 1)
        initial_s0[i] = i;

    auto stage_cache_vbuf = vec2buf(initial_s0);
    auto data_initial_vbuf = vec2buf(unit_data_all, false);

    cl::Buffer stage_mem[6];
    for(int i = 0 ; i < 6 ; i += 1) 
        stage_mem[i] = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_int) * unit_data_all.size());

    cl::Buffer data_mem(context, CL_MEM_READ_WRITE, 
                        sizeof(unit_data) * view_dir.size(), nullptr);

    // Random seeds
    std::vector<cl_long> random_seeds(view_dir.size());
    for(size_t i = 0 ; i < view_dir.size() ; i += 1)
        random_seeds[i] = rand();
    auto rand_seed_vbuf = vec2buf(random_seeds, false);

    // Compute first level intersect and cache stage_s1
    cl_int v_sizes[10] = {0};
    v_sizes[0] = view_dir.size();
    queue.enqueueWriteBuffer(sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

#define WORK_GROUP_SIZE 256

#define ND_RANGE(queue, kernel, global) \
    (queue).enqueueNDRangeKernel((kernel), 0, \
                                 (((global) / (WORK_GROUP_SIZE) + 1) * (WORK_GROUP_SIZE)), \
                                 (WORK_GROUP_SIZE))

    cl::Kernel kernel(program, "kdtree_intersect");
    setKernelArgs(kernel, 
                  sizes_mem, 
                  data_initial_vbuf.first,
                  stage_cache_vbuf.first,
                  stage_mem[1],
                  points_vbuf.first,
                  geometries_vbuf.first,
                  kdtree_leaf_data_vbuf.first,
                  kdtree_node_vbuf.first,
                  kdtree_node_vbuf.second);
    ND_RANGE(queue, kernel, view_dir.size());
    // queue.enqueueNDRangeKernel(kernel, 0, view_dir.size(), 256);
    queue.finish();

    queue.enqueueReadBuffer(sizes_mem, CL_FALSE, 0, sizeof(cl_int) * 10, v_sizes);
    queue.enqueueCopyBuffer(stage_mem[1], stage_cache_vbuf.first, 0, 0, sizeof(cl_int) * unit_data_all.size());
    queue.finish();
    auto intial_s1_size = v_sizes[1];

    uint64_t s0_time = 0;
    uint64_t s1_time = 0;
    uint64_t s2_time = 0;

    for(int ii = 0 ; ii < sample ; ii += 1) {

        TickTock timer;

        memset(v_sizes, 0, sizeof(v_sizes));
        v_sizes[1] = intial_s1_size;

        queue.enqueueWriteBuffer(sizes_mem, CL_FALSE, 0, sizeof(cl_int) * 10, v_sizes);
        queue.enqueueCopyBuffer(data_initial_vbuf.first, data_mem, 0, 0, sizeof(unit_data) * unit_data_all.size());
        queue.enqueueCopyBuffer(stage_cache_vbuf.first, stage_mem[1], 0, 0, sizeof(cl_int) * unit_data_all.size());
        queue.finish();

        for(int i = 0 ; i < depth ; i += 1) {
            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);

            auto t0 = GetTimeStamp();

            cl::Kernel kernel(program, "kdtree_intersect");
            setKernelArgs(kernel,
                          sizes_mem,
                          data_mem,
                          stage_mem[0],
                          stage_mem[1],
                          points_vbuf.first,
                          geometries_vbuf.first,
                          kdtree_leaf_data_vbuf.first,
                          kdtree_node_vbuf.first,
                          kdtree_node_vbuf.second);
            if(v_sizes[0])
                ND_RANGE(queue, kernel, v_sizes[0]);
                // queue.enqueueNDRangeKernel(kernel, 0, v_sizes[0], 256);
            queue.finish();

            queue.enqueueReadBuffer(sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
            v_sizes[0] = 0;
            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            // queue.enqueueWriteBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

            auto t1 = GetTimeStamp();

            // run S1
            kernel = cl::Kernel(program, "s1_run");
            setKernelArgs(kernel,
                          sizes_mem,
                          data_mem,
                          stage_mem[1],
                          results_mem,
                          stage_mem[2],
                          stage_mem[3],
                          stage_mem[4],
                          stage_mem[5],
                          points_vbuf.first,
                          normals_vbuf.first,
                          normals_vbuf.second,
                          texcoords_vbuf.first,
                          texcoords_vbuf.second,
                          materials_vbuf.first,
                          texture_mem,
                          rand_seed_vbuf.first);
            if(v_sizes[1])
                ND_RANGE(queue, kernel, v_sizes[1]);
                // queue.enqueueNDRangeKernel(kernel, 0, v_sizes[1], 256);
            queue.finish();

            queue.enqueueReadBuffer(sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
            v_sizes[0] = 0;
            v_sizes[1] = 0;
            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            queue.enqueueWriteBuffer(sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

            if(i == depth - 1) break;

            auto t2 = GetTimeStamp();

            cl::Kernel kernels[4];
            kernels[0] = cl::Kernel(program, "s2_refract_run");
            kernels[1] = cl::Kernel(program, "s2_specular_run");
            kernels[2] = cl::Kernel(program, "s2_diffuse_run");
            kernels[3] = cl::Kernel(program, "s2_light_run");
            for(int i = 0 ; i < 4 ; i += 1) {
                kernels[i].setArg(0, sizes_mem);
                kernels[i].setArg(1, data_mem);
                kernels[i].setArg(2, stage_mem[i+2]);
                kernels[i].setArg(3, stage_mem[0]);
                if(i == 1 || i == 2)
                    kernels[i].setArg(4, rand_seed_vbuf.first);
            }
            kernels[3].setArg(4, lights_vbuf.first);
            kernels[3].setArg(5, lights_vbuf.second);
            kernels[3].setArg(6, points_vbuf.first);
            kernels[3].setArg(7, rand_seed_vbuf.first);

            for(int i = 0 ; i < 4 ; i += 1)
                if(v_sizes[i+2])
                    ND_RANGE(queue, kernels[i], v_sizes[i+2]);
                    // queue.enqueueNDRangeKernel(kernels[i], 0, v_sizes[i+2], 256);
            queue.finish();

            queue.enqueueReadBuffer(sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
            v_sizes[2] = v_sizes[3] = v_sizes[4] = v_sizes[5] = 0;
            queue.enqueueWriteBuffer(sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

            auto t3 = GetTimeStamp();

            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);

            s0_time += t1 - t0;
            s1_time += t2 - t1;
            s2_time += t3 - t2;
        }

        timer.timeit("Sample %d done", ii);
    }

    hp_log("Time: s0 %llu, s1 %llu, s2 %llu", s0_time, s1_time, s2_time);

    std::vector<cl_float> result(view_dir.size() * 3, 0);
    // result.resize(view_dir.size() * 3, 0);
    queue.enqueueReadBuffer(results_mem, CL_TRUE, 0, sizeof(cl_float) * view_dir.size() * 3, result.data());

    for(size_t i = 0 ; i < result.size() ; i += 1)
        result[i] /= float(sample);

    return result;
}
