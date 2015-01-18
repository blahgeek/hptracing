/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-17
*/

#include <iostream>
#include "./trace_runner_cl.h"
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <sys/time.h>

using namespace hp;

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

void cl::TraceRunner::run() {
    std::srand(std::time(0));

    std::vector<cl::unit_data> s0_all;
    for(size_t i = 0 ; i < view_dir.size() ; i += 1) {
        cl::unit_data x;
        x.orig_id = i;
        x.strength.s[0] = x.strength.s[1] = x.strength.s[2] = 1;
        x.strength.s[3] = 0;
        x.start_p = view_p;
        x.in_dir = view_dir[i];
        s0_all.push_back(x);
    }
    // Aha! magic!
    // std::random_shuffle(s0_all.begin(), s0_all.end());

    auto kdtree_data = scene->getData();

    // Scene related data, readonly
    cl_mem points_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 sizeof(cl_float3) * scene->points.size(),
                                                 scene->points.data());
    // cl_int geometries_size = scene->geometries.size();
    cl_mem kdtree_node_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                      sizeof(cl::KDTreeNodeHeader) * kdtree_data.first.size(),
                                                      kdtree_data.first.data());
    cl_int kdtree_node_size = kdtree_data.first.size();
    cl_mem kdtree_leaf_data_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                           sizeof(cl_int) * kdtree_data.second.size(),
                                                           kdtree_data.second.data());
    // geometries_size /= 10;
    cl_mem geometries_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                     sizeof(cl_int4) * scene->geometries.size(),
                                                     scene->geometries.data());
    cl_int lights_size = scene->lights.size();
    cl_mem lights_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 sizeof(cl_int4) * scene->lights.size(),
                                                 scene->lights.data());
    cl_mem materials_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                    sizeof(cl::Material) * scene->materials.size(),
                                                    scene->materials.data());

    // Result data
    cl_mem results_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                  sizeof(cl_float) * view_dir.size() * 3, nullptr);

    // Units...
    cl_mem v_sizes_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                  sizeof(cl_int) * 10, nullptr);

    cl_int * intial_s0 = new cl_int[view_dir.size()];
    for(int i = 0 ; i < view_dir.size() ; i += 1)
        intial_s0[i] = i;

    // #define UNIT_DATA_SIZE 1000000
    cl_mem v_data_initial_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                                         sizeof(cl::unit_data) * view_dir.size(), s0_all.data());
    cl_mem s0_initial_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                                     sizeof(cl_int) * view_dir.size(), intial_s0);

    cl_mem v_data_mem = cl_program->createBuffer(CL_MEM_READ_WRITE, 
                                                 sizeof(cl::unit_data) * view_dir.size(), nullptr);
    cl_mem s0_mem = cl_program->createBuffer(CL_MEM_READ_WRITE, 
                                             sizeof(cl_int) * view_dir.size(), nullptr);
    cl_mem s1_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                             sizeof(cl_int) * view_dir.size(), nullptr);
    cl_mem s2_refract_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl_int) * view_dir.size(), nullptr);
    cl_mem s2_specular_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl_int) * view_dir.size(), nullptr);
    cl_mem s2_diffuse_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl_int) * view_dir.size(), nullptr);
    cl_mem s2_light_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                   sizeof(cl_int) * view_dir.size(), nullptr);

#define SAMPLES 10
    // Random seeds
    cl_long * random_seeds = new cl_long[view_dir.size()];
    for(int i = 0 ; i < view_dir.size() ; i += 1)
        random_seeds[i] = rand();
    cl_mem rand_seed_mem = cl_program->createBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                                                    sizeof(cl_long) * view_dir.size(), random_seeds);

    int max_data = -1;
    int max_sizes[10] = {0};

    uint64_t s0_time = 0;
    uint64_t s1_time = 0;
    uint64_t s2_time = 0;

    for(size_t ii = 0 ; ii < SAMPLES ; ii += 1) {
        cl_int v_sizes[10] = {0};
        v_sizes[0] = view_dir.size();
        cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
        // cl_program->writeBuffer(v_data_mem, 
        //                         sizeof(cl::unit_data) * s0_all.size(), 
        //                         s0_all.data());

        // cl_program->writeBuffer(s0_mem, sizeof(cl_int) * view_dir.size(), intial_s0);
        clEnqueueCopyBuffer(cl_program->commands, v_data_initial_mem, v_data_mem, 0, 0, sizeof(cl::unit_data) * view_dir.size(), 0, NULL, NULL);
        clEnqueueCopyBuffer(cl_program->commands, s0_initial_mem, s0_mem, 0, 0, sizeof(cl_int) * view_dir.size(), 0, NULL, NULL);
        clFinish(cl_program->commands);

#define MAX_DEPTH 6
        for(int i = 0 ; i < MAX_DEPTH ; i += 1) {
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);

            size_t local = 256;

            auto t0 = GetTimeStamp();

            // run S0
            // auto kernel = cl_program->getKernel("naive_intersect");
            // clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
            // clSetKernelArg(kernel, 1, sizeof(cl_mem), &v_data_mem);
            // clSetKernelArg(kernel, 2, sizeof(cl_mem), &s0_mem);
            // clSetKernelArg(kernel, 3, sizeof(cl_mem), &s1_mem);
            // clSetKernelArg(kernel, 4, sizeof(cl_mem), &points_mem);
            // clSetKernelArg(kernel, 5, sizeof(cl_mem), &geometries_mem);
            // clSetKernelArg(kernel, 6, sizeof(cl_int), &geometries_size);
            auto kernel = cl_program->getKernel("kdtree_intersect");
            clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel, 1, sizeof(cl_mem), &v_data_mem);
            clSetKernelArg(kernel, 2, sizeof(cl_mem), &s0_mem);
            clSetKernelArg(kernel, 3, sizeof(cl_mem), &s1_mem);
            clSetKernelArg(kernel, 4, sizeof(cl_mem), &points_mem);
            clSetKernelArg(kernel, 5, sizeof(cl_mem), &geometries_mem);
            clSetKernelArg(kernel, 6, sizeof(cl_mem), &kdtree_leaf_data_mem);
            clSetKernelArg(kernel, 7, sizeof(cl_mem), &kdtree_node_mem);
            clSetKernelArg(kernel, 8, sizeof(cl_int), &kdtree_node_size);

            // local = 2;
            // size_t global = 2;
            // global = (v_sizes[0] / local + 1) * local;
            cl_program->enqueueNDKernel(kernel, v_sizes[0] );
            // cl_program->enqueueNDKernel(kernel, 5);
            // clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
            clFinish(cl_program->commands);
            cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            if(v_sizes[0] > max_sizes[0]) max_sizes[0] = v_sizes[0];
            v_sizes[0] = 0;
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            clReleaseKernel(kernel);

            auto t1 = GetTimeStamp();

            // run S1
            kernel = cl_program->getKernel("s1_run");
            clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel, 1, sizeof(cl_mem), &v_data_mem);
            clSetKernelArg(kernel, 2, sizeof(cl_mem), &s1_mem);
            clSetKernelArg(kernel, 3, sizeof(cl_mem), &results_mem);
            clSetKernelArg(kernel, 4, sizeof(cl_mem), &s2_refract_mem);
            clSetKernelArg(kernel, 5, sizeof(cl_mem), &s2_specular_mem);
            clSetKernelArg(kernel, 6, sizeof(cl_mem), &s2_diffuse_mem);
            clSetKernelArg(kernel, 7, sizeof(cl_mem), &s2_light_mem);
            clSetKernelArg(kernel, 8, sizeof(cl_mem), &points_mem);
            clSetKernelArg(kernel, 9, sizeof(cl_mem), &materials_mem);
            clSetKernelArg(kernel, 10, sizeof(cl_mem), &rand_seed_mem);

            // global = (v_sizes[1] / local + 1) * local;
            cl_program->enqueueNDKernel(kernel, v_sizes[1]);
            // clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
            clFinish(cl_program->commands);
            cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            if(v_sizes[1] > max_sizes[1]) max_sizes[1] = v_sizes[1];
            v_sizes[1] = 0;
            // v_sizes[6] = (i % 2 == 0) ? (UNIT_DATA_SIZE / 2) : 0;
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            clReleaseKernel(kernel);

            if(i == MAX_DEPTH - 1) break;

            auto t2 = GetTimeStamp();

            // run S2 refract
            auto kernel_refract = cl_program->getKernel("s2_refract_run");
            clSetKernelArg(kernel_refract, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_refract, 1, sizeof(cl_mem), &v_data_mem);
            clSetKernelArg(kernel_refract, 2, sizeof(cl_mem), &s2_refract_mem);
            clSetKernelArg(kernel_refract, 3, sizeof(cl_mem), &s0_mem);
            // global = (v_sizes[2] / local + 1) * local;
            cl_program->enqueueNDKernel(kernel_refract, v_sizes[2]);
            // clEnqueueNDRangeKernel(cl_program->commands, kernel_refract, 1, NULL, &global, &local, 0, NULL, NULL);
            // run S2 specular
            auto kernel_specular = cl_program->getKernel("s2_specular_run");
            clSetKernelArg(kernel_specular, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_specular, 1, sizeof(cl_mem), &v_data_mem);
            clSetKernelArg(kernel_specular, 2, sizeof(cl_mem), &s2_specular_mem);
            clSetKernelArg(kernel_specular, 3, sizeof(cl_mem), &s0_mem);
            // global = (v_sizes[3] / local + 1) * local;
            cl_program->enqueueNDKernel(kernel_specular, v_sizes[3]);
            // clEnqueueNDRangeKernel(cl_program->commands, kernel_specular, 1, NULL, &global, &local, 0, NULL, NULL);
            // run S2 diffuse 
            auto kernel_diffuse = cl_program->getKernel("s2_diffuse_run");
            clSetKernelArg(kernel_diffuse, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_diffuse, 1, sizeof(cl_mem), &v_data_mem);
            clSetKernelArg(kernel_diffuse, 2, sizeof(cl_mem), &s2_diffuse_mem);
            clSetKernelArg(kernel_diffuse, 3, sizeof(cl_mem), &s0_mem);
            clSetKernelArg(kernel_diffuse, 4, sizeof(cl_mem), &rand_seed_mem);
            // global = (v_sizes[4] / local + 1) * local;
            cl_program->enqueueNDKernel(kernel_diffuse, v_sizes[4]);
            // clEnqueueNDRangeKernel(cl_program->commands, kernel_diffuse, 1, NULL, &global, &local, 0, NULL, NULL);
            // run S2 light 
            auto kernel_light = cl_program->getKernel("s2_light_run");
            clSetKernelArg(kernel_light, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_light, 1, sizeof(cl_mem), &v_data_mem);
            clSetKernelArg(kernel_light, 2, sizeof(cl_mem), &s2_light_mem);
            clSetKernelArg(kernel_light, 3, sizeof(cl_mem), &s0_mem);
            clSetKernelArg(kernel_light, 4, sizeof(cl_mem), &lights_mem);
            clSetKernelArg(kernel_light, 5, sizeof(cl_int), &lights_size);
            clSetKernelArg(kernel_light, 6, sizeof(cl_mem), &points_mem);
            clSetKernelArg(kernel_light, 7, sizeof(cl_mem), &rand_seed_mem);
            // global = (v_sizes[5] / local + 1) * local;
            cl_program->enqueueNDKernel(kernel_light, v_sizes[5]);
            // clEnqueueNDRangeKernel(cl_program->commands, kernel_light, 1, NULL, &global, &local, 0, NULL, NULL);
            // finish S2
            clFinish(cl_program->commands);
            cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            if(v_sizes[2] > max_sizes[2]) max_sizes[2] = v_sizes[2];
            if(v_sizes[3] > max_sizes[3]) max_sizes[3] = v_sizes[3];
            if(v_sizes[4] > max_sizes[4]) max_sizes[4] = v_sizes[4];
            if(v_sizes[5] > max_sizes[5]) max_sizes[5] = v_sizes[5];
            v_sizes[2] = v_sizes[3] = v_sizes[4] = v_sizes[5] = 0;
            cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);

            auto t3 = GetTimeStamp();

            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d, Data %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5], v_sizes[6]);
            clReleaseKernel(kernel_refract);
            clReleaseKernel(kernel_specular);
            clReleaseKernel(kernel_diffuse);
            // clReleaseKernel(kernel_light);

            s0_time += t1 - t0;
            s1_time += t2 - t1;
            s2_time += t3 - t2;

            // auto tmp = v_sizes[6] - ((i % 2 == 0) ? (UNIT_DATA_SIZE / 2) : 0);
            // if(tmp > max_data) max_data = tmp;
        }
    }

    // hp_log("Max data: %d", max_data);
    hp_log("Max sx size: %d %d %d %d %d %d", 
           max_sizes[0], max_sizes[1], max_sizes[2], max_sizes[3],
           max_sizes[4], max_sizes[5]);
    hp_log("Time: s0 %ld, s1 %ld, s2 %ld", s0_time, s1_time, s2_time);

    result.resize(view_dir.size() * 3, 0);
    cl_program->readBuffer(results_mem, sizeof(cl_float) * view_dir.size() * 3, result.data());

    for(int i = 0 ; i < result.size() ; i += 1)
        result[i] /= float(SAMPLES);
}
