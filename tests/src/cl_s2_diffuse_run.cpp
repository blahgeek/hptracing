/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-11
*/

#include <iostream>

#include <gtest/gtest.h>
#include <fstream>

#include "./hp/common.h"
#include "./hp/cl_program.h"
#include "./hp/scene_cl/base.h"
#include <sys/time.h>

using namespace std;
using namespace hp;

TEST(CLTest, s2_diffuse_run) {
    auto cl_program = std::make_unique<hp::CLProgram>();

    auto kernel = cl_program->getKernel("s2_diffuse_run");
    cl_mem v_sizes_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                  sizeof(cl_int) * 10, nullptr);
    cl_mem s0_mem = cl_program->createBuffer(CL_MEM_READ_WRITE, 
                                             sizeof(cl::unit_S0) * 500000, nullptr);
    cl_mem s2_diffuse_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_diffuse) * 100000, nullptr);
    // Random seeds
    cl_long * random_seeds = new cl_long[100000];
    for(int i = 0 ; i < 100000 ; i += 1)
        random_seeds[i] = rand();
    cl_mem rand_seed_mem = cl_program->createBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                                                    sizeof(cl_long) * 100000, random_seeds);


#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
    } while(0)

    cl::unit_S2_diffuse unit;
    unit.orig_id = 0;
    unit.depth = 0;
    ASSIGN_F3(unit.new_strength, Vec(1,1,1));
    // (191.123154 548.799988 235.960510) -> (0.337651 -0.731054 0.592918)
    ASSIGN_F3(unit.in_dir, Vec(0.337651, -0.731054, 0.592918));
    ASSIGN_F3(unit.normal, Vec(0,-1,0));
    Vec start_p = Vec(191.123154, 548.799988, 235.960510);
    Vec intersect_p = start_p + Vec(0.337651, -0.731054, 0.592918) * 1.573924;
    ASSIGN_F3(unit.intersect_p, intersect_p);

    cl_program->writeBuffer(s2_diffuse_mem, sizeof(unit), &unit);

    cl_int v_sizes[10] = {0};
    v_sizes[4] = 1;
    cl_program->writeBuffer(v_sizes_mem, sizeof(v_sizes), v_sizes);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &s2_diffuse_mem);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &s0_mem);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &rand_seed_mem);

    size_t global = 100;
    size_t local = 10;
    clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
    clFinish(cl_program->commands);

    cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);

    cl::unit_S0 s0[16];
    cl_program->readBuffer(s0_mem, sizeof(cl::unit_S0) * 16, s0);

    __builtin_trap();

}
