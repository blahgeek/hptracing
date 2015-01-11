/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-11
*/

#include <iostream>
#include "./trace_runner_cl.h"
#include <cstdlib>
#include <algorithm>
#include <ctime>

using namespace hp;

void cl::TraceRunner::run() {
    std::srand(std::time(0));

    std::vector<cl::unit_S0> s0_all;
    for(size_t i = 0 ; i < view_dir.size() ; i += 1) {
        cl::unit_S0 x;
        x.orig_id = i;
        x.depth = 0;
        x.strength.s[0] = x.strength.s[1] = x.strength.s[2] = 1;
        x.strength.s[3] = 0;
        x.start_p = view_p;
        x.in_dir = view_dir[i];
        s0_all.push_back(x);
    }
    // std::random_shuffle(s0_all.begin(), s0_all.end());

    // Scene related data, readonly
    cl_mem points_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 sizeof(cl_float3) * scene->points.size(),
                                                 scene->points.data());
    cl_int geometries_size = scene->geometries.size();
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
    cl_mem s0_mem = cl_program->createBuffer(CL_MEM_READ_WRITE, 
                                             sizeof(cl::unit_S0) * 500000, nullptr);
    cl_mem s1_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                             sizeof(cl::unit_S1) * 100000, nullptr);
    cl_mem s2_refract_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_refract) * 100000, nullptr);
    cl_mem s2_specular_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_specular) * 100000, nullptr);
    cl_mem s2_diffuse_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_diffuse) * 100000, nullptr);
    cl_mem s2_light_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                   sizeof(cl::unit_S2_light) * 100000, nullptr);

#define RAY_PER_LOOP 5000
    // Random seeds
    cl_long * random_seeds = new cl_long[500000];
    for(int i = 0 ; i < 500000 ; i += 1)
        random_seeds[i] = rand();
    cl_mem rand_seed_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                                    sizeof(cl_long) * 500000, random_seeds);

    // cl::unit_S1 * test_s1 = new cl::unit_S1[100000];

    for(int ii = 0 ; ii < s0_all.size() ; ii += RAY_PER_LOOP) {
        cl_int v_sizes[10] = {0};
        v_sizes[0] = RAY_PER_LOOP;
        cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
        cl_program->writeBuffer(s0_mem, 
                                sizeof(cl::unit_S0) * std::min(RAY_PER_LOOP, int(s0_all.size() - ii)), 
                                &(s0_all[ii]));
        for(int i = 0 ; i < 4 ; i += 1) {
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5]);
            size_t global = 0, local = 200;

            // run S0
            auto kernel = cl_program->getKernel("naive_intersect");
            clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel, 1, sizeof(cl_mem), &s0_mem);
            clSetKernelArg(kernel, 2, sizeof(cl_mem), &s1_mem);
            clSetKernelArg(kernel, 3, sizeof(cl_mem), &points_mem);
            clSetKernelArg(kernel, 4, sizeof(cl_mem), &geometries_mem);
            clSetKernelArg(kernel, 5, sizeof(cl_int), &geometries_size);

            global = (v_sizes[0] / local + 1) * local;
            clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
            clFinish(cl_program->commands);
            cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            v_sizes[0] = 0;
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5]);
            cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            clReleaseKernel(kernel);

            // // test
            // if(i == 1) {
            //     cl_program->readBuffer(s1_mem, sizeof(cl::unit_S1) * 100000, test_s1);
            //     for(int i = 0 ; i < v_sizes[1] ; i += 1) {
            //         auto s1 = test_s1[i];
            //         if(s1.start_p.s[1] > 500 && s1.geometry.s[3] == 5) { // ceiling -> light
            //             hp_log("(%f %f %f) -> (%f %f %f) : %f, strenth(%f %f %f)",
            //                    s1.start_p.s[0], s1.start_p.s[1], s1.start_p.s[2],
            //                    s1.in_dir.s[0], s1.in_dir.s[1], s1.in_dir.s[2],
            //                    s1.intersect_number,
            //                    s1.strength.s[0], s1.strength.s[1], s1.strength.s[2]);
            //         }
            //     }
            // }

            // run S1
            kernel = cl_program->getKernel("s1_run");
            clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel, 1, sizeof(cl_mem), &s1_mem);
            clSetKernelArg(kernel, 2, sizeof(cl_mem), &results_mem);
            clSetKernelArg(kernel, 3, sizeof(cl_mem), &s2_refract_mem);
            clSetKernelArg(kernel, 4, sizeof(cl_mem), &s2_specular_mem);
            clSetKernelArg(kernel, 5, sizeof(cl_mem), &s2_diffuse_mem);
            clSetKernelArg(kernel, 6, sizeof(cl_mem), &s2_light_mem);
            clSetKernelArg(kernel, 7, sizeof(cl_mem), &points_mem);
            clSetKernelArg(kernel, 8, sizeof(cl_mem), &materials_mem);

            global = (v_sizes[1] / local + 1) * local;
            clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
            clFinish(cl_program->commands);
            cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            v_sizes[1] = 0;
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5]);
            cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            clReleaseKernel(kernel);

            // run S2 refract
            auto kernel_refract = cl_program->getKernel("s2_refract_run");
            clSetKernelArg(kernel_refract, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_refract, 1, sizeof(cl_mem), &s2_refract_mem);
            clSetKernelArg(kernel_refract, 2, sizeof(cl_mem), &s0_mem);
            global = (v_sizes[2] / local + 1) * local;
            clEnqueueNDRangeKernel(cl_program->commands, kernel_refract, 1, NULL, &global, &local, 0, NULL, NULL);
            // run S2 specular
            auto kernel_specular = cl_program->getKernel("s2_specular_run");
            clSetKernelArg(kernel_specular, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_specular, 1, sizeof(cl_mem), &s2_specular_mem);
            clSetKernelArg(kernel_specular, 2, sizeof(cl_mem), &s0_mem);
            global = (v_sizes[3] / local + 1) * local;
            clEnqueueNDRangeKernel(cl_program->commands, kernel_specular, 1, NULL, &global, &local, 0, NULL, NULL);
            // run S2 diffuse 
            auto kernel_diffuse = cl_program->getKernel("s2_diffuse_run");
            clSetKernelArg(kernel_diffuse, 0, sizeof(cl_mem), &v_sizes_mem);
            clSetKernelArg(kernel_diffuse, 1, sizeof(cl_mem), &s2_diffuse_mem);
            clSetKernelArg(kernel_diffuse, 2, sizeof(cl_mem), &s0_mem);
            clSetKernelArg(kernel_diffuse, 3, sizeof(cl_mem), &rand_seed_mem);
            global = (v_sizes[4] / local + 1) * local;
            clEnqueueNDRangeKernel(cl_program->commands, kernel_diffuse, 1, NULL, &global, &local, 0, NULL, NULL);
            // // run S2 light 
            // auto kernel_light = cl_program->getKernel("s2_light_run");
            // clSetKernelArg(kernel_light, 0, sizeof(cl_mem), &v_sizes_mem);
            // clSetKernelArg(kernel_light, 1, sizeof(cl_mem), &s2_light_mem);
            // clSetKernelArg(kernel_light, 2, sizeof(cl_mem), &s0_mem);
            // clSetKernelArg(kernel_light, 3, sizeof(cl_mem), &lights_mem);
            // clSetKernelArg(kernel_light, 4, sizeof(cl_int), &lights_size);
            // clSetKernelArg(kernel_light, 5, sizeof(cl_mem), &points_mem);
            // clSetKernelArg(kernel_light, 6, sizeof(cl_mem), &rand_seed_mem);
            // global = (v_sizes[5] / local + 1) * local;
            // clEnqueueNDRangeKernel(cl_program->commands, kernel_light, 1, NULL, &global, &local, 0, NULL, NULL);
            // finish S2
            clFinish(cl_program->commands);
            cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            v_sizes[2] = v_sizes[3] = v_sizes[4] = v_sizes[5] = 0;
            hp_log("Loop%d: Size: S0 %d, S1 %d, S2 %d %d %d %d", i, v_sizes[0], v_sizes[1], v_sizes[2], v_sizes[3], v_sizes[4], v_sizes[5]);
            cl_program->writeBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);
            clReleaseKernel(kernel_refract);
            clReleaseKernel(kernel_specular);
            clReleaseKernel(kernel_diffuse);
            // clReleaseKernel(kernel_light);
        }
    }

    result.resize(view_dir.size() * 3, 0);
    cl_program->readBuffer(results_mem, sizeof(cl_float) * view_dir.size() * 3, result.data());
}
