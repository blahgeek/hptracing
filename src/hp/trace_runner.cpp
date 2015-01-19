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

TraceRunner::TraceRunner(std::unique_ptr<hp::KDTree> && scene,
                         std::vector<cl_float3> && view_dir,
                         cl_float3 view_p, int sample, int depth) :
scene(std::move(scene)), view_dir(std::move(view_dir)), view_p(view_p),
sample(sample), depth(depth) {
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

    for(size_t i = 0 ; i < this->view_dir.size() ; i += 1) {
        unit_data x;
        x.orig_id = i;
        x.strength.s[0] = x.strength.s[1] = x.strength.s[2] = 1;
        x.strength.s[3] = 0;
        x.start_p = view_p;
        x.in_dir = this->view_dir[i];
        unit_data_all.push_back(x);
    }

    // Aha! magic!
    // std::random_shuffle(unit_data_all.begin(), unit_data_all.end());
}

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

void TraceRunner::run() {
    cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);

    auto kdtree_data = scene->getData();

    // Scene related data, readonly
    cl::Buffer points_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(cl_float3) * scene->points.size(), scene->points.data());
    cl::Buffer kdtree_node_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                               sizeof(KDTreeNodeHeader) * kdtree_data.first.size(), kdtree_data.first.data());
    cl_int kdtree_node_size = kdtree_data.first.size();
    cl::Buffer kdtree_leaf_data_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    sizeof(cl_int) * kdtree_data.second.size(), kdtree_data.second.data());
    cl::Buffer geometries_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              sizeof(cl_int4) * scene->geometries.size(),
                              scene->geometries.data());
    cl_int lights_size = scene->lights.size();
    cl::Buffer lights_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          sizeof(cl_int4) * scene->lights.size(), scene->lights.data());

#ifdef CL_VERSION_1_2
    cl::Image2DArray texture_mem;
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
    cl::Buffer texture_mem;
    if(scene->texture_names.size() > 0)
        texture_mem = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 4 * Scene::texture_width * Scene::texture_height * scene->texture_names.size(),
                                 scene->texture_data.data());
#endif

    cl_int texcoords_size = scene->texcoords.size();
    cl::Buffer texcoords_mem;
    if(texcoords_size > 0)
        texcoords_mem = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(cl_float2) * texcoords_size, scene->texcoords.data());

    cl::Buffer materials_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             sizeof(Material) * scene->materials.size(),
                             scene->materials.data());
    cl_int normals_size = scene->normals.size();
    cl::Buffer normals_mem;
    if(normals_size > 0)
        normals_mem = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 sizeof(cl_float3) * normals_size, scene->normals.data());

    // Result data
    cl::Buffer results_mem(context, CL_MEM_READ_WRITE,
                           sizeof(cl_float) * view_dir.size() * 3, nullptr);

    // Units...
    cl::Buffer v_sizes_mem(context, CL_MEM_READ_WRITE,
                           sizeof(cl_int) * 10, nullptr);

    cl_int * intial_s0 = new cl_int[view_dir.size()];
    for(size_t i = 0 ; i < view_dir.size() ; i += 1)
        intial_s0[i] = i;

    cl::Buffer stage_cache_mem(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                              sizeof(cl_int) * view_dir.size(), intial_s0);
    cl::Buffer v_data_initial_mem(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                                  sizeof(unit_data) * unit_data_all.size(), 
                                  unit_data_all.data());

    cl::Buffer stage_mem[6];
    for(int i = 0 ; i < 6 ; i += 1) 
        stage_mem[i] = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_int) * unit_data_all.size());

    cl::Buffer v_data_mem(context, CL_MEM_READ_WRITE, 
                          sizeof(unit_data) * view_dir.size(), nullptr);

    // Random seeds
    cl_long * random_seeds = new cl_long[view_dir.size()];
    for(size_t i = 0 ; i < view_dir.size() ; i += 1)
        random_seeds[i] = rand();
    cl::Buffer rand_seed_mem(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                             sizeof(cl_long) * view_dir.size(), random_seeds);

    // Compute first level intersect and cache stage_s1
    cl_int v_sizes[10] = {0};
    v_sizes[0] = view_dir.size();
    queue.enqueueWriteBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
    // queue.enqueueCopyBuffer(v_data_initial_mem, v_data_mem, 0, 0, sizeof(unit_data) * unit_data_all.size());
    // queue.enqueueCopyBuffer(stage_cache_mem, stage_mem[0], 0, 0, sizeof(cl_int) * unit_data_all.size());
    // queue.finish();

#define WORK_GROUP_SIZE 256

#define ND_RANGE(queue, kernel, global) \
    (queue).enqueueNDRangeKernel((kernel), 0, \
                                 (((global) / (WORK_GROUP_SIZE) + 1) * (WORK_GROUP_SIZE)), \
                                 (WORK_GROUP_SIZE))

    cl::Kernel kernel(program, "kdtree_intersect");
    kernel.setArg(0, v_sizes_mem);
    kernel.setArg(1, v_data_initial_mem);
    kernel.setArg(2, stage_cache_mem); // s0
    kernel.setArg(3, stage_mem[1]); // s1
    kernel.setArg(4, points_mem);
    kernel.setArg(5, geometries_mem);
    kernel.setArg(6, kdtree_leaf_data_mem);
    kernel.setArg(7, kdtree_node_mem);
    kernel.setArg(8, kdtree_node_size);
    ND_RANGE(queue, kernel, view_dir.size());
    // queue.enqueueNDRangeKernel(kernel, 0, view_dir.size(), 256);
    queue.finish();

    queue.enqueueReadBuffer(v_sizes_mem, CL_FALSE, 0, sizeof(cl_int) * 10, v_sizes);
    queue.enqueueCopyBuffer(stage_mem[1], stage_cache_mem, 0, 0, sizeof(cl_int) * unit_data_all.size());
    queue.finish();
    auto intial_s1_size = v_sizes[1];

    uint64_t s0_time = 0;
    uint64_t s1_time = 0;
    uint64_t s2_time = 0;

    for(int ii = 0 ; ii < this->sample ; ii += 1) {

        TickTock timer;

        memset(v_sizes, 0, sizeof(v_sizes));
        v_sizes[1] = intial_s1_size;

        queue.enqueueWriteBuffer(v_sizes_mem, CL_FALSE, 0, sizeof(cl_int) * 10, v_sizes);
        queue.enqueueCopyBuffer(v_data_initial_mem, v_data_mem, 0, 0, sizeof(unit_data) * unit_data_all.size());
        queue.enqueueCopyBuffer(stage_cache_mem, stage_mem[1], 0, 0, sizeof(cl_int) * unit_data_all.size());
        queue.finish();

        for(int i = 0 ; i < this->depth ; i += 1) {
            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);

            auto t0 = GetTimeStamp();

            cl::Kernel kernel(program, "kdtree_intersect");
            kernel.setArg(0, v_sizes_mem);
            kernel.setArg(1, v_data_mem);
            kernel.setArg(2, stage_mem[0]); // s0
            kernel.setArg(3, stage_mem[1]); // s1
            kernel.setArg(4, points_mem);
            kernel.setArg(5, geometries_mem);
            kernel.setArg(6, kdtree_leaf_data_mem);
            kernel.setArg(7, kdtree_node_mem);
            kernel.setArg(8, kdtree_node_size);
            if(v_sizes[0])
                ND_RANGE(queue, kernel, v_sizes[0]);
                // queue.enqueueNDRangeKernel(kernel, 0, v_sizes[0], 256);
            queue.finish();

            queue.enqueueReadBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
            v_sizes[0] = 0;
            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            // queue.enqueueWriteBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

            auto t1 = GetTimeStamp();

            // run S1
            kernel = cl::Kernel(program, "s1_run");
            kernel.setArg(0, v_sizes_mem);
            kernel.setArg(1, v_data_mem);
            kernel.setArg(2, stage_mem[1]);
            kernel.setArg(3, results_mem);
            for(int i = 2 ; i < 6 ; i += 1)
                kernel.setArg(i+2, stage_mem[i]);
            kernel.setArg(8, points_mem);
            kernel.setArg(9, normals_mem);
            kernel.setArg(10, normals_size);
            kernel.setArg(11, texcoords_mem);
            kernel.setArg(12, texcoords_size);
            kernel.setArg(13, materials_mem);
            kernel.setArg(14, texture_mem);
            kernel.setArg(15, rand_seed_mem);
            if(v_sizes[1])
                ND_RANGE(queue, kernel, v_sizes[1]);
                // queue.enqueueNDRangeKernel(kernel, 0, v_sizes[1], 256);
            queue.finish();

            queue.enqueueReadBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
            v_sizes[0] = 0;
            v_sizes[1] = 0;
            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            queue.enqueueWriteBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

            if(i == this->depth - 1) break;

            auto t2 = GetTimeStamp();

            cl::Kernel kernels[4];
            kernels[0] = cl::Kernel(program, "s2_refract_run");
            kernels[1] = cl::Kernel(program, "s2_specular_run");
            kernels[2] = cl::Kernel(program, "s2_diffuse_run");
            kernels[3] = cl::Kernel(program, "s2_light_run");
            for(int i = 0 ; i < 4 ; i += 1) {
                kernels[i].setArg(0, v_sizes_mem);
                kernels[i].setArg(1, v_data_mem);
                kernels[i].setArg(2, stage_mem[i+2]);
                kernels[i].setArg(3, stage_mem[0]);
                if(i == 1 || i == 2)
                    kernels[i].setArg(4, rand_seed_mem);
            }
            kernels[3].setArg(4, lights_mem);
            kernels[3].setArg(5, lights_size);
            kernels[3].setArg(6, points_mem);
            kernels[3].setArg(7, rand_seed_mem);

            for(int i = 0 ; i < 4 ; i += 1)
                if(v_sizes[i+2])
                    ND_RANGE(queue, kernels[i], v_sizes[i+2]);
                    // queue.enqueueNDRangeKernel(kernels[i], 0, v_sizes[i+2], 256);
            queue.finish();

            queue.enqueueReadBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);
            v_sizes[2] = v_sizes[3] = v_sizes[4] = v_sizes[5] = 0;
            queue.enqueueWriteBuffer(v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes);

            auto t3 = GetTimeStamp();

            // hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);

            s0_time += t1 - t0;
            s1_time += t2 - t1;
            s2_time += t3 - t2;
        }

        timer.timeit("Sample %d done", ii);
    }

    hp_log("Time: s0 %llu, s1 %llu, s2 %llu", s0_time, s1_time, s2_time);

    result.resize(view_dir.size() * 3, 0);
    queue.enqueueReadBuffer(results_mem, CL_TRUE, 0, sizeof(cl_float) * view_dir.size() * 3, result.data());

    for(size_t i = 0 ; i < result.size() ; i += 1)
        result[i] /= float(this->sample);
}
